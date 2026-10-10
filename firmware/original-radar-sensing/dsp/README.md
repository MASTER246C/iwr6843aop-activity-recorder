# Original radar DSP baseline

This allocation-free C99 implementation is independently authored from basic FFT,
windowing, energy and peak-selection mathematics. It does not use TI demo DPC/DPU,
CFAR, GTRACK, Capon, people-counting, posture classification, DSPLIB or HWA
algorithms. TI's compiler/runtime math may supply `sinf`, `cosf`, `memset` and
ordinary compiler intrinsics. All FFT orchestration and arithmetic here is source
code in `radar_dsp.c`. No HWA or DSP acceleration is enabled.

It produces coarse spectral measurements, not validated person detections or a
sitting/walking/standing classifier. A static wall and a stationary person can
both produce zero-Doppler peaks. Highest-power selection can omit a weaker
person, and the global background threshold is not a calibrated false-alarm-rate
CFAR detector. No accuracy claim is made without labeled hardware measurements.

## Capture and processing contract

- Baseline: one fixed TX, hardware RX0 through RX3, 64 complex ADC samples per
  chirp, 32 uniformly spaced coherent chirps, nominal 250 us chirp interval.
  The integrated `room64_v1` RF profile uses approximately 30 MHz/us slope and
  evaluated range bins4..30. The self-contained synthetic tests also exercise
  a 60 MHz/us example; all physical scales are supplied by configuration.
- Buffer: `[chirp][logical RX][sample]`, each `RadarComplex16` has signed 16-bit
  `real` followed by `imag`. For the little-endian TI target this is ADCBUF
  `sampleInterleave=0`, `channelInterleave=1`. Input is I+jQ. Do not silently swap
  components, conjugate Q, reorder RX, or mix TDM-MIMO TX chirps.
- `physical_rx[]` explicitly identifies the hardware channel for each logical RX.
  The array is carried by configuration, not estimated from the samples.
- The caller owns coherence, chirp counts, DMA/ADCBUF completion and immutability
  during processing. The sample count must exactly match N*C*R. Partial frames
  are rejected; do not fill missing chirps with zeros and mark them complete.
- The caller must hold separate storage for state, input, metadata and result.
  State is deliberately public for bounded profile access; do not change its
  configuration/twiddles/workspace except by reinitialization and processing.
- `RadarDsp` must be static or in explicitly reserved memory, not on a small RTOS
  task stack. There is no malloc/free, recursion or global mutable DSP state.

Each frame applies a periodic Hann range window normalized to unit sum, then the
original forward radix-two FFT. Only configured positive-frequency bins between
1 and N/2-1 are evaluated. No fast-time DC subtraction or slow-time clutter
removal is performed. The per-range slow-time mean and mean-squared magnitude are
retained before the second Hann window. The original slow-time FFT keeps signed
Doppler bins including zero. RX powers combine noncoherently, while each RX's
complex coefficient remains available for separately calibrated angle work.

The gate is `max(minimum_power, noise_multiplier * median(evaluated 2-D powers))`.
A candidate must strictly exceed the gate and be a local maximum in a 3x3
range/Doppler neighborhood (Doppler wraps, range does not). Equal adjacent maxima
have a deterministic lower-index tie break. The strongest eight maxima are
returned. This bounded selection is original and intentionally simple. Hann
mainlobes and two unresolved reflectors can merge; an adjacent slow Doppler
component can be hidden by strong static leakage.

## Units and calibration

A bin-centered isolated complex tone of amplitude A ADC counts has peak `power`
approximately A squared, independent of receiver count. Quantization and floating
point introduce small error. These are uncalibrated ADC-count-squared values,
not dBm, RCS, probability, confidence, occupancy or distance accuracy.

`range_total_power` is the RX-mean, time-mean power of the range coefficient.
`range_static_power` is the RX-mean squared coherent slow-time mean.
`range_dynamic_power = max(total - static, 0)` is temporal variance; it includes
noise and multipath and is not uniquely human motion. An ideal repeated static
return remains detectable and has near-zero dynamic power. Static information
is never deliberately erased.

Range bin spacing is c*Fs/(2*slope*N). Velocity bin spacing is
c/(2*carrier*C*chirp_interval). Use the actual programmed RF slope, sampling rate
and chirp interval, and preferably RF carrier at the ADC-window center. Signed
velocity follows the mathematical I+jQ convention. Physical toward/away sign
must be checked on hardware before `motion_sign_verified` is set.

RX phase and absolute power are always marked uncalibrated. `rx_phasor[]` values
are normalized coherent 2-D coefficients in logical RX order, without phase
calibration, angle estimation or MIMO motion compensation. Range bias is not
calibrated either. Mount geometry, the 15-degree down tilt and the unknown signed
vertical antenna-center offset from the 2 m bracket top belong in the separate
spatial stage. This DSP never silently substitutes zero for that unknown offset.

The integrated 30 MHz/us profile with Fs=2.5 MHz yields approximately 0.19518 m
bins; selected bins4..30 cover roughly 0.78..5.86 m bin centers. Use actual
quantized RF constants for exact values. The analog HPF settings can strongly
attenuate near-range returns (the RF integration documents this limitation).

At the synthetic-test Fs=2.5 MHz and slope=60 MHz/us, N=64 yields ~0.097589 m bins
and a maximum reported bin center ~3.0253 m (bin31). Other complex-ADC spectral
conventions can permit different mappings, but they require RF/IF validation;
this implementation does not silently reinterpret the negative half as range.
At 61.378 GHz carrier and 250 us chirp interval, C=32 gives ~0.305273 m/s Doppler
bins and an aliased edge around +/-4.88436 m/s. That resolution does not establish
low-motion posture. At 2 m height and 15 degrees down, the boresight-floor
intersection would be ~7.46 m horizontally before antenna offset, outside this
conservative reported-range interval for either profile. Off-boresight returns
can be visible, but this geometry is not a full-floor coverage guarantee. The RF
profile and mount must suit the target area.

## Full profile retained for host learning/calibration

Top-K results are only a convenient bounded summary. After a successful call,
the complete evaluated profile remains in:

- `state.total_power[r]`, `state.static_power[r]`; dynamic is max(total-static,0)
- `state.power[r][d]`, the RX-mean 2-D range/Doppler power
- `state.cube[r][logical_rx][d]`, the coherent per-RX coefficient

Only configured ranges and receivers are meaningful. Doppler array index d maps
to d for d<C/2 and d-C otherwise. These arrays describe the most recent successful
frame; an error output has no detections and must not be associated with old
profile memory. A future host logger can collect profiles as well as peaks to
avoid learning only dominant furniture. Exporting the full cube needs its own
bandwidth budget and is not done here.

## Missing frames and quality

A complete frame is self-contained; no phase is combined across frames. An
unsigned 32-bit sequence counter supports normal wrap. Forward gaps flag and
count missing frames; duplicates and backward/ambiguous half-range jumps reject
the frame. Incomplete or wrong-length frames reject without updating sequence.
A capture error does not convert old data into a new movement estimate.

The integration must provide actual frame sequence numbers, not merely increment
on successful processing, or acquisition losses cannot be identified by this
API. Timestamps are copied, not used to infer chirp timing. Clipped signed-16-bit
components are counted and flag the frame, but detection remains possible.

## Bounded resource cost

Default limits N=64, C=32, R=4 give:

- `sizeof(RadarDsp)`: 42,552 bytes on the tested host ABI; confirm target linker
- `sizeof(RadarDspResult)`: 560 bytes; full input capture: 32,768 bytes
- Range FFTs: 32*4*(64/2)*log2(64) = 24,576 complex butterflies
- Doppler FFTs: 31*4*(32/2)*log2(32) = 9,920 complex butterflies
- Total FFT butterflies: 34,496, plus window/power loops and 992-bin median
- Median uses an original iterative heapsort, bounded O(B log B), no recursion
- Local peak selection examines at most eight neighbors for each of 992 bins;
  the insertion list never exceeds eight entries
- All trigonometric calls happen during initialization, not frame processing

N and C are configurable powers of two within compile-time bounds. Smaller
profiles reduce work but retain the same statically reserved maximum workspace.
Raising `RADAR_DSP_MAX_SAMPLES` to128 doubles most workspace; recompile every
consumer with identical macros. There are no wall-clock real-time guarantees
from host timing. Measure R4F cycle count under actual capture/RTOS/telemetry load
and reject overruns before increasing frame rate. A 200 MHz core has 40 million
cycles per 200 ms frame, but interrupt and DMA latency also matter.

## Reproducible checks

From this directory:

    make test
    make sanitize
    make arm-check TI_CGT_ROOT=/path/to/ti-arm-cgt

The host build uses strict C99 warnings as errors. ASan and UBSan are enabled in
`sanitize`; leak scanning is disabled because the execution environment runs
under ptrace and LeakSanitizer cannot operate there. This implementation has no
heap allocation. The independent long-double direct DFT oracle checks the FFT;
a separate direct 2-D sum checks actual quantized multi-target and noisy captures
without reusing FFT code or generated window tables. Tests also cover retained
static energy, positive/negative/Nyquist Doppler, RX phase, non-integer bins,
noise, clipping, invalid configuration, top-K bounds, one-bin range selection,
partial frames, exact lengths, sequence gaps, duplicates, stale frames and wrap.

`arm-check` compiles with TI ARM16.9.6.LTS for Cortex-R4F Thumb/VFPv3D16 at O2;
it is a portability check, not a hardware run or firmware flash. The enclosing
firmware build must also link the object and verify total linker RAM placement.
