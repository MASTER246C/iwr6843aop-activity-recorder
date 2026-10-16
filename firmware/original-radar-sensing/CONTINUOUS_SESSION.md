# Supervised continuous session,0.3.2-rc1

This contract is unchanged in the unreleased 0.3.3-dev source apart from the first-cause fault precedence noted below. That source is not built with the TI toolchain, not flashed and not physically tested. The 0.3.2-rc1 candidate remains the accepted binary. It later ran one supervised continuous session on hardware on 2026-10-12 ([first combined run](../../docs/FIRST_COMBINED_RUN_2026-10-12.md)): 283 frames over 56.4 seconds, 53 keepalives, then manual Stop. Deliberate keepalive-loss/disconnect Stop and endurance were not tested there.

`session continuous` explicitly selects recurring 5 Hz FMCW frames. Bounded `session 1` through `session 5` keep their finite behavior. `session 0` is still rejected; internal seconds=0 is only produced by the explicit continuous token. Configuration/pairing never start RF. A fresh cross-port nonce probe, confirmation and reviewed status precede a separate manual Start.

The matching viewer sends one serialized control command approximately every 1000 ms during a continuous run:

```
keepalive <32-lowercase-hex nonce> <uint32 config_generation> <uint32 session_id>
```

Only an exact active-session tuple renews the 5000 ms rolling lease. Code 0 ACKs include the current `pair_nonce`, `config_generation`, `session_id` and `keepalive_tick_ms` (raw 1 ms firmware tick of the last accepted renewal). Wrong, malformed, expired, pre-Start, bounded-mode or post-Stop requests return code-34 without extending the lease. Rejected ACKs describe the current firmware tuple; they never attest the supplied invalid tuple. A canceled operation or lost ACK remains uncertain; delayed ACKs cannot reauthorize it or restart RF.

Lease expiry, missing active pairing or lease revocation stops through the watchdog with fault 463. In 0.3.3-dev source, an earlier latched cause (for example 484 data transport, which also revokes pairing) is kept instead of being replaced by 463. Manual Stop revokes and stops. RF/acquisition/UART faults, missing-frame detection and processing deadlines remain enabled. The viewer must Stop and cease keepalives on either-port disconnect, malformed data, stale data or tuple mismatch. Healthy keepalives impose no total-duration cap. UART transmission success does not establish host consumption. A frozen RTOS or blocked callback prevents a hard wall-clock Stop guarantee, as in the prior implementation.

Status, probes and frames include `session_mode` (`continuous` or `bounded`), `session_seconds` (0 or 1–5), and `keepalive_lease_ms` (5000 or 0). Protocol remains original-radar-v 2; an exact version/build match selects a compatible viewer. The old v1 Python collector cannot control this firmware.

Frames additionally identify `timestamp_basis:"session_elapsed"`. `timestamp_us` is session elapsed time since acquisition arm. Completion callbacks extend the 32-bit 1 ms RTOS clock with unsigned differences into a 64-bit elapsed accumulator, then multiply by 1000 in 64-bit arithmetic. Healthy frames arrive every 200 ms, well within one raw-clock wrap. Timestamp conversion exceeds the approximately 71.6-minute 32-bit microsecond range and remains monotonic across the approximately 49.7-day raw-clock rollover. It resets on each arm; nonce/session tuples distinguish sessions. Sequence arithmetic retains unsigned wrap handling.

Firmware keeps two fixed ADC buffers and bounded DSP/record memory, independent of run duration. Host recording/storage is a separate concern. The existing exploratory stream does not establish complete final-frame drainage, physical calibration, people count or activity recognition. Long physical operation, deadlines, stack headroom and thermal behavior remain unmeasured.

`config/continuous_session.cfg.template` stays intentionally invalid until measured tilt replaces its placeholder. Retain uncertain antenna offset as unknown and do not invent phase/axis calibration. First physical checks are exact identity/self-test/status, no-RF pairing, manual Stop and lease-loss Stop, followed by a supervised sensing run. Build and fake tests do not establish those checks. The 2026-10-12 run covered identity/self-test, pairing, one supervised continuous run and manual Stop on 0.3.2-rc1 only; lease-loss Stop remains untested on hardware.
