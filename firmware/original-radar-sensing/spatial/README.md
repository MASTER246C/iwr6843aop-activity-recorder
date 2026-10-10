# Original exploratory 2×2 spatial estimator

`original_spatial.c` is independently written mathematical interferometry, not a TI demo detection, tracking, Capon, or AoA DPU. The only board facts used are IWR6843AOP RX offsets `(1,1), (1,0), (0,1), (0,0)` and 2.5 mm pitch, documented in SDK 3.6.2 `ti/board/antenna_geometry.c` and `.h`.

For a selected range/Doppler cell, two adjacent horizontal RX phase differences and two vertical differences are summed as complex correlations. Their arguments yield direction cosines with the configured physical carrier and pitch. The output assumes one dominant reflector per cell. A conservative direction-cosine/FOV gate and pair-coherence gate reject unsupported estimates; these do not establish accuracy in multipath or mixed-target scenes. Four receivers provide a small aperture and coarse directional information only.

## Trust and mounting

The defaults explicitly set receiver phase calibration and axis orientation to unverified. Returned sensor coordinates are exploratory measurements, not calibrated ground truth. Physical angle signs must be checked with a known target; per-RX complex corrections must be measured. Common phase does not matter, but receiver-dependent bias does.

The current reported floor-to-bracket-flat-top height is 1.2192 m (4 ft), with tilt still unconfirmed. The positive antenna offset is only estimated at 0.003175–0.00635 m. Antenna height is `measured bracket height + signed vertical antenna-center offset`, where the offset is defined **after** the physical tilt and is unknown by default. It must not be guessed from the bracket measurement or multiplied by another cosine. Absolute world height remains NaN until the offset, receiver calibration, and axis orientation are known. Serial output must represent unknown values with JSON `null`, never nonstandard NaN or a guessed zero.

Rotation convention: sensor x is horizontal, sensor y is forward, sensor z is up after axis validation. Downward tilt rotates `y_world = y_sensor*cos(tilt) + z_sensor*sin(tilt)` and `z_world = antenna_height + z_sensor*cos(tilt) - y_sensor*sin(tilt)`.

A scatterer's height is not automatically the person's head or posture. No sitting/standing/walking classifier is implemented here. Synthetic correctness is distinct from hardware calibration or human-activity accuracy.

## Offline validation

Host tests cover 937 direction pairs, boresight, signed height offset, 15° tilt, mandatory unknown-height behavior, large amplitudes, missing/zero data, invalid numeric inputs, low pair coherence, and FOV rejection. These tests synthesize mathematical plane waves; no board or physical signals were measured.
