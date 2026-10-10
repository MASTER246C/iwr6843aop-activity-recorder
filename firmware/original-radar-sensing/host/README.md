# Original-firmware JSONL collector

This separate collector is for `original-radar-v1` at 115200 baud, not TI TLVs or the existing v0.5/v0.6 TI capture/learning workflow. It preserves session labels, raw coherent per-RX phasors, selected-bin total/static/variance energy summaries, range/velocity/power, calibration flags, null geometry, packet faults and timing. It does not infer posture or improve accuracy by itself. Firmware, UART operation and RF sensing remain hardware-unverified.

## Prepare Python

Use Python 3.10 or newer. Create a virtual environment if desired and install `python -m pip install -r host/requirements.txt`. pyserial 3.5 is the only serial dependency. Offline replay/tests use only the Python standard library. No auto-discovery chooses or opens a board port. The collector never flashes or modifies boot switches.

## Passive collection

After an independently authorized firmware/board bring-up, select its explicit UART port:

```
python host/original_capture.py --port YOUR_PORT --output sitting_001.jsonl --label sitting --duration 65
```

Without command flags this only reads the selected UART. An output file must not already exist. Labels are supplied by the person conducting the recording; they are not algorithm predictions. Use separate sessions for empty-room background, sitting, standing, walking, and transitions. Keep furniture, mounting and antenna offset provenance; do not silently merge differently calibrated datasets.

## Configure without starting RF

```
python host/original_capture.py --port YOUR_PORT --configure config/room64_v1.cfg --output configured_001.jsonl --duration 2
```

Before configuration the collector sends a harmless `status` query and requires a correctly tagged, command-correlated original-protocol acknowledgement. TI firmware or an unknown protocol fails closed. Each configure-only line must be acknowledged; a rejection or timeout stops further commands. The config validator explicitly rejects `start`, `stop` and unknown command families in config files. Config alone never requests RF calibration or transmission.

## Explicit start, only after hardware bring-up is approved

```
python host/original_capture.py --port YOUR_PORT --configure config/room64_v1.cfg --start --output walking_001.jsonl --label walking --duration 65
```

`--start` explicitly requests RF calibration/transmission. Do not run this against unreviewed/unvalidated hardware simply because the image compiled. The supplied board config has a finite 60-second run, independent of the host duration. If this collector requested start, it attempts stop on exit/interruption even if start acknowledgement was lost. A failed stop acknowledgement is logged rather than called successful. `--stop` separately requests stopping a pre-existing original-firmware run; it is never sent in passive mode.

Serial port opening sets DTR/RTS false before opening, but physical USB bridge behavior remains untested. The port is exclusive on POSIX when supported. The collector does not have an automatic reconnect/restart policy.

## Offline replay and tests

```
python host/original_capture.py --input raw_original_uart.jsonl --output replay_001.jsonl --label unknown
python -m unittest discover -s host -p 'test_*.py' -v
```

Replay accepts raw UART JSONL lines, not the wrapped capture file it produces, and labels its source `replay`; it cannot send commands. The supplied test frame is synthetic and never evidence of board operation. Test coverage includes every byte-split boundary, malformed/non-finite/oversized lines, frame counts, unknown-height invariants, command correlation, refusal to send start from a config, timeouts, sequence wrap/gaps, no-overwrite behavior and offline replay.

## File format

The new JSONL file starts with `schema:"original-radar-jsonl-capture", version:1`, distinct from the TI-derived capture schema. It records source kind, user label, config content/hash, and optional user-supplied BIN hash. The hash is provenance, not attestation of the running device. Each accepted UART line becomes a `board_record` envelope with UTC reception time, elapsed host time, label and unchanged decoded record. Invalid lines retain bounded base64 bytes and a reason. The summary counts frames, invalid/oversized/partial lines and host-observed sequence gaps.

Calibration must be respected. Default antenna height is unknown: the measured bracket flat top is 2 m and tilt is 15 degrees, but the signed vertical antenna-center offset after tilt is not known. No zero-offset substitution is allowed. RX phase and coordinate axes are also unverified. Exploratory sensor coordinates are not calibrated TI points. A supplied user label may still be useful ground truth for later dedicated training; this collector intentionally does not pass the data into a TI-trained model.

The additive `profile_energy:[total,static,variance]` frame field preserves aggregate selected-range-bin measurements even when the three strongest serialized peaks are stationary. These are uncalibrated mean-square/coherent-mean-square/residual-variance summaries, not probabilities or physical object classes. See `PROTOCOL.md` for exact normalization.
