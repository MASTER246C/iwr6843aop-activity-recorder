#!/usr/bin/env python3
"""Dedicated original-radar-v1 collector. Never flashes; passive unless requested.

This is deliberately NOT a TI TLV decoder or a trained activity classifier.
"""
from __future__ import annotations
import argparse
import base64
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import re
import sys
import time
from typing import Any, BinaryIO

PROTOCOL = "original-radar-v1"
CAPTURE_SCHEMA = "original-radar-jsonl-capture"
MAX_LINE = 4096
STATES = {"idle", "starting", "running", "stopping", "fault"}
LABELS = ("unknown", "empty", "sitting", "standing", "walking", "transition")

class CaptureError(ValueError):
    pass

def reject_constant(token: str) -> None:
    raise CaptureError(f"Non-finite JSON number {token}")

def finite_json_float(token: str) -> float:
    value = float(token)
    if not math.isfinite(value):
        raise CaptureError("Non-finite JSON exponent")
    return value

def integer(value: Any, low: int, high: int, field: str) -> None:
    if type(value) is not int or not low <= value <= high:
        raise CaptureError(f"Invalid {field}")

def number(value: Any, field: str) -> None:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or (isinstance(value, float) and not math.isfinite(value)) or (isinstance(value, int) and abs(value) > 10**100):
        raise CaptureError(f"Invalid {field}")

def vector(value: Any, field: str, allow_null: bool = True) -> None:
    if value is None and allow_null:
        return
    if not isinstance(value, list) or len(value) != 3:
        raise CaptureError(f"Invalid {field}")
    for item in value:
        number(item, field)

def decode_record(raw: bytes) -> dict[str, Any]:
    if len(raw) > MAX_LINE:
        raise CaptureError("Oversized UART line")
    try:
        record = json.loads(raw.decode("utf-8"), parse_constant=reject_constant, parse_float=finite_json_float)
    except (UnicodeError, json.JSONDecodeError, ValueError, RecursionError) as exc:
        raise CaptureError(f"Invalid JSON: {exc}") from exc
    pending = [(record, 0)]
    while pending:
        value, depth = pending.pop()
        if depth > 32:
            raise CaptureError("Excessively nested JSON")
        if isinstance(value, dict):
            pending.extend((item, depth + 1) for item in value.values())
        elif isinstance(value, list):
            pending.extend((item, depth + 1) for item in value)
        elif type(value) is int and abs(value) > 10**100:
            raise CaptureError("Oversized JSON integer")
    if not isinstance(record, dict) or record.get("protocol") != PROTOCOL:
        raise CaptureError("Not original-radar-v1; do not use this collector for TI TLVs")
    kind = record.get("type")
    if kind not in {"identity", "ack", "status", "frame", "error"}:
        raise CaptureError("Unknown record type")
    if kind == "ack":
        integer(record.get("code"), -2147483648, 2147483647, "ack code")
        if record.get("state") not in STATES or type(record.get("configured")) is not bool:
            raise CaptureError("Invalid ack state")
    if kind != "frame":
        return record
    for key in ("sequence", "missed_frames", "capture_fault", "quality", "processing_us",
                "previous_tx_us", "deadline_misses"):
        integer(record.get(key), 0, 0xFFFFFFFF, key)
    integer(record.get("timestamp_us"), 0, 0xFFFFFFFFFFFFFFFF, "timestamp_us")
    if record.get("complete") is not True:
        raise CaptureError("Incomplete frame cannot be accepted as a coherent measurement")
    integer(record.get("count"), 0, 3, "count")
    integer(record.get("detected_count"), record["count"], 8, "detected_count")
    if type(record.get("truncated")) is not bool or record["truncated"] != (record["count"] < record["detected_count"]):
        raise CaptureError("Inconsistent truncation flag")
    if not isinstance(record.get("detections"), list) or len(record["detections"]) != record["count"]:
        raise CaptureError("Detection count mismatch")
    for key in ("range_bin_m", "velocity_bin_mps"):
        number(record.get(key), key)
        if record[key] <= 0:
            raise CaptureError(f"Invalid {key}")
    if "profile_energy" in record:
        vector(record["profile_energy"], "profile_energy", allow_null=False)
        if any(item < 0 for item in record["profile_energy"]):
            raise CaptureError("Negative profile energy")
    mount = record.get("mount")
    if not isinstance(mount, dict) or type(mount.get("world_height_valid")) is not bool:
        raise CaptureError("Missing mount trust status")
    number(mount.get("bracket_top_m"), "bracket_top_m")
    number(mount.get("down_tilt_deg"), "down_tilt_deg")
    if not 0 < mount["bracket_top_m"] <= 5 or not -45 <= mount["down_tilt_deg"] <= 45:
        raise CaptureError("Mount fields exceed configured firmware bounds")
    for key in ("antenna_offset_m", "antenna_height_m"):
        if mount.get(key) is not None:
            number(mount[key], key)
    if mount.get("antenna_offset_m") is None:
        if mount.get("antenna_height_m") is not None or mount["world_height_valid"]:
            raise CaptureError("Unknown antenna offset cannot produce known absolute height")
    elif not -1 <= mount["antenna_offset_m"] <= 1:
        raise CaptureError("Antenna offset exceeds configured firmware bounds")
    elif mount.get("antenna_height_m") is not None:
        expected = mount["bracket_top_m"] + mount["antenna_offset_m"]
        if not math.isclose(mount["antenna_height_m"], expected, rel_tol=1e-4, abs_tol=1e-4):
            raise CaptureError("Antenna height must use signed vertical offset after tilt")
    if mount["world_height_valid"] and (mount.get("antenna_height_m") is None or mount["antenna_height_m"] <= 0):
        raise CaptureError("World height validity requires positive known antenna height")
    for det in record["detections"]:
        if not isinstance(det, dict):
            raise CaptureError("Invalid detection")
        for key in ("range_m", "velocity_mps", "power"):
            number(det.get(key), key)
        if det["range_m"] < 0 or det["power"] < 0:
            raise CaptureError("Negative range or power")
        integer(det.get("spatial_flags"), 0, 0xFFFFFFFF, "spatial_flags")
        phasors = det.get("phasors")
        if not isinstance(phasors, list) or len(phasors) != 4:
            raise CaptureError("Exactly four RX phasors are required")
        for pair in phasors:
            if not isinstance(pair, list) or len(pair) != 2:
                raise CaptureError("Invalid RX phasor")
            number(pair[0], "phasor real"); number(pair[1], "phasor imaginary")
        vector(det.get("sensor_xyz_m"), "sensor_xyz_m")
        vector(det.get("world_xyz_m"), "world_xyz_m")
        if det.get("world_xyz_m") is not None and (not mount["world_height_valid"] or (det["spatial_flags"] & 28) != 28):
            raise CaptureError("World point conflicts with unknown mount/calibration")
        if det.get("activity") is not None:
            raise CaptureError("This measurement protocol does not implement activity inference")
    return record

def load_config(path: Path) -> list[str]:
    commands: list[str] = []
    for raw in path.read_text(encoding="ascii").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        if len(line) >= 192 or any(ord(c) < 32 or ord(c) > 126 for c in line):
            raise CaptureError("Invalid config line")
        parts = line.split()
        if parts[0] not in {"orConfig", "profile", "mount", "detect", "session", "commit"}:
            raise CaptureError("Configuration may not include RF start/stop or unknown commands")
        commands.append(" ".join(parts))
    if not commands or commands[0] != "orConfig 1" or commands[-1] != "commit":
        raise CaptureError("Configuration must begin orConfig 1 and end commit")
    if len(commands) > 32:
        raise CaptureError("Too many config commands")
    return commands

class Framer:
    """Bounded newline framing; overlong lines are discarded through newline."""
    def __init__(self) -> None:
        self.buffer = bytearray()
        self.discarding = False
        self.oversized = 0
    def feed(self, data: bytes) -> list[bytes]:
        lines: list[bytes] = []
        for b in data:
            if self.discarding:
                if b == 10:
                    self.discarding = False
                continue
            self.buffer.append(b)
            if len(self.buffer) > MAX_LINE:
                self.buffer.clear(); self.discarding = b != 10; self.oversized += 1
            elif b == 10:
                if self.buffer.strip():
                    lines.append(bytes(self.buffer))
                self.buffer.clear()
        return lines

class Recorder:
    def __init__(self, output: Path, label: str, source_kind: str,
                 config: Path | None = None, firmware_sha256: str | None = None) -> None:
        self.file = output.open("x", encoding="utf-8")  # Never overwrite a capture.
        self.started = time.monotonic()
        self.label = label
        self.frames = self.invalid = self.records = self.sequence_gaps = 0
        self.last_sequence: int | None = None
        self.write({"type":"capture_header", "schema":CAPTURE_SCHEMA, "version":1,
                    "source_protocol":PROTOCOL, "source_kind":source_kind,
                    "label":label, "label_source":"user_supplied" if label != "unknown" else "unknown",
                    "firmware_sha256_user_supplied":firmware_sha256,
                    "config_text":config.read_text() if config else None,
                    "config_sha256":hashlib.sha256(config.read_bytes()).hexdigest() if config else None,
                    "build_hardware_validation":"unverified", "classifier":False,
                    "calibration":"preserve_each_board_record; never assume TI calibration"})
    def write(self, value: dict[str, Any]) -> None:
        envelope = {"received_utc":dt.datetime.now(dt.timezone.utc).isoformat(), **value}
        self.file.write(json.dumps(envelope, allow_nan=False, separators=(",", ":"))+"\n")
        self.file.flush()
    def accept(self, raw: bytes) -> dict[str, Any] | None:
        try:
            record = decode_record(raw)
        except CaptureError as exc:
            self.invalid += 1
            self.write({"type":"invalid_line", "reason":str(exc),
                        "raw_base64":base64.b64encode(raw[:MAX_LINE]).decode("ascii")})
            return None
        self.records += 1
        wrapper: dict[str, Any] = {"type":"board_record", "elapsed_s":time.monotonic()-self.started,
                                   "label":self.label, "record":record}
        if record["type"] == "frame":
            seq = record["sequence"]
            if self.last_sequence is not None:
                delta = (seq-self.last_sequence) & 0xFFFFFFFF
                if delta == 0 or delta >= 0x80000000:
                    wrapper["host_sequence_status"]="duplicate_or_regression"
                elif delta > 1:
                    self.sequence_gaps += delta-1
                    wrapper["host_sequence_gap"] = delta-1
            self.last_sequence = seq
            self.frames += 1
        self.write(wrapper)
        return record
    def finish(self, reason: str, oversized: int = 0, partial_bytes: int = 0) -> None:
        self.write({"type":"capture_summary", "reason":reason, "frames":self.frames,
                    "records":self.records, "invalid_lines":self.invalid,
                    "host_sequence_gaps":self.sequence_gaps,"oversized_lines":oversized,
                    "partial_line_bytes":partial_bytes})
        self.file.close()

def read_available(stream: Any, framer: Framer, recorder: Recorder) -> list[dict[str, Any]]:
    data = stream.read(512)
    return [r for line in framer.feed(data) if (r := recorder.accept(line)) is not None]

def command(stream: Any, line: str, framer: Framer, recorder: Recorder,
            timeout: float = 15.0) -> dict[str, Any]:
    # Calls only use literal control commands or a separately validated config.
    recorder.write({"type":"host_command", "command":line})
    encoded = (line+"\n").encode("ascii")
    if stream.write(encoded) != len(encoded):
        raise CaptureError("Incomplete command write")
    stream.flush()
    deadline = time.monotonic()+timeout
    while time.monotonic() < deadline:
        for record in read_available(stream,framer,recorder):
            if record["type"] == "ack" and record.get("command") == line.split()[0]:
                if record["code"] < 0 or record["state"] == "fault":
                    raise CaptureError(f"Firmware rejected {line!r}: {record}")
                return record
    raise CaptureError(f"No original-radar-v1 acknowledgement for {line!r}; no further commands sent")

def replay(stream: BinaryIO, recorder: Recorder) -> Framer:
    framer=Framer()
    while True:
        data=stream.read(8192)
        if not data: break
        for line in framer.feed(data): recorder.accept(line)
    return framer

def main(argv: list[str] | None = None) -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    source=parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--port", help="Explicit original-firmware UART port; never auto-select")
    source.add_argument("--input", type=Path, help="Offline raw UART JSONL replay; sends no commands")
    parser.add_argument("--output",required=True,type=Path,help="New capture JSONL path; must not exist")
    parser.add_argument("--label",choices=LABELS,default="unknown")
    parser.add_argument("--configure",type=Path,help="Explicitly load configure-only original .cfg")
    parser.add_argument("--start",action="store_true",help="Explicitly request RF start after protocol/config validation")
    parser.add_argument("--stop",action="store_true",help="Explicitly stop an existing original session, then capture")
    parser.add_argument("--duration",type=float,default=65.0,help="Host capture seconds, 0<seconds<=660")
    parser.add_argument("--firmware-sha256",help="Optional known BIN hash; recorded as user-supplied, not device-attested")
    args=parser.parse_args(argv)
    if args.input and (args.configure or args.start or args.stop): parser.error("Replay does not accept board commands")
    if not math.isfinite(args.duration) or not 0<args.duration<=660: parser.error("duration must be finite and between 0 and 660 seconds")
    if args.firmware_sha256 and not re.fullmatch(r"[0-9a-fA-F]{64}",args.firmware_sha256): parser.error("firmware hash must be 64 hex characters")
    commands=load_config(args.configure) if args.configure else []
    recorder=Recorder(args.output,args.label,"replay" if args.input else "serial",args.configure,args.firmware_sha256)
    framer=Framer(); stream=None; started_by_us=False; reason="complete"; result=0
    try:
        if args.input:
            with args.input.open("rb") as f: framer=replay(f,recorder)
        else:
            try: import serial
            except ImportError as exc: raise CaptureError("Install pyserial>=3.5 in your Python environment; no board was opened") from exc
            options={"baudrate":115200,"timeout":0.2,"write_timeout":3.0}
            if os.name=="posix": options["exclusive"]=True
            stream=serial.Serial(port=None,**options)
            stream.dtr=False; stream.rts=False; stream.port=args.port; stream.open()
            if args.configure or args.start or args.stop:
                ack=command(stream,"status",framer,recorder)
                if args.stop:
                    ack=command(stream,"stop",framer,recorder)
                for line in commands:
                    ack=command(stream,line,framer,recorder)
                if args.start:
                    if not ack["configured"] or ack["state"] != "idle":
                        raise CaptureError("Start requires confirmed configured, idle original firmware")
                    print("Requesting RF start explicitly. This build is not hardware-validated.",file=sys.stderr)
                    started_by_us=True  # Stop on cleanup even if RF start ACK is lost.
                    command(stream,"start",framer,recorder)
            deadline=time.monotonic()+args.duration
            while time.monotonic()<deadline:
                read_available(stream,framer,recorder)
    except KeyboardInterrupt:
        reason="interrupted"; result=130
    except Exception as exc:
        reason=f"error: {exc}"; result=1
        recorder.write({"type":"host_error","error":str(exc)})
        print(reason,file=sys.stderr)
    finally:
        if stream is not None:
            if started_by_us:
                try:
                    if not stream.is_open:
                        raise CaptureError("Serial port closed before Stop could be confirmed")
                    command(stream,"stop",framer,recorder)
                except Exception as exc:
                    recorder.write({"type":"stop_not_confirmed","error":str(exc)})
                    # Preserve an earlier failure/interrupt while refusing false success.
                    if result == 0:
                        reason="stop_not_confirmed"; result=1
                        print(f"Stop not confirmed: {exc}",file=sys.stderr)
            try: stream.close()
            except Exception as exc: recorder.write({"type":"serial_close_error","error":str(exc)})
        recorder.finish(reason,framer.oversized,len(framer.buffer))
    print(f"Saved {recorder.frames} frame(s), {recorder.invalid} invalid line(s) to {args.output}")
    return result

if __name__=="__main__":
    raise SystemExit(main())
