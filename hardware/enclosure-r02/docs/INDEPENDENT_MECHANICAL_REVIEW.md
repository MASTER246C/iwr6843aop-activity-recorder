# R02 simplified enclosure: independent mechanical review

Checked 12 October 2026, 09:18 UTC. Applies to the five-functional-part revision with lower front-screw clearance relief, PCB front plane Z8.0 mm, and integral radar membrane inner surface Z4.1 mm. R01 is superseded for this review.

## Digital checks

- Five functional parts: no pairwise solid-volume overlaps above 0.001 mm³.
- Twelve installed common thumb screws: no overlaps above that threshold. The two lower front screws were rechecked after adding clearance behind their threaded bosses. Other geometry was unchanged by that material-removal fix.
- Full, unrounded 94 × 43.3 × 71 mm camera envelope, including clip: no collisions with those five parts at the modeled pose.
- The 1,437-solid classified AOP reference, rotated with a proper 180° Y rotation, clears all five parts. Checks used exact geometry cropped to the complete device bounds plus a 1 mm margin, followed by per-solid spatial filtering and Boolean intersections. This preserves all possible device/part collision regions.
- Integral radar membrane is modeled as 0.30 mm through the clear aperture; no additional camera glazing is modeled.

## What the device references establish

The official Rev G STEP contains another board model as well as the AOP board. The research reference isolates probable AOP solids by documented placement and height. It is useful engineering evidence, but is not a manufacturer-certified complete AOP-only assembly. The finished-board STEP contour measures approximately 115.268 × 25.272 mm; nominal fabrication dimensions are approximately 115.01 × 25.02 mm. STEP board thickness is approximately 1.604 mm, versus nominal 1.62 mm.

The mounting posts use the two documented 2.54 mm holes at approximately 35.8237 mm pitch. The board/component reference has no collision with the posts. The actual heatsink, the user's different U-fold bracket, plugged connectors and cable bend envelopes are not established by this pass.

Camera evidence is limited to Logitech's published overall dimensions including clip. There is no verified clip articulation, optical-plane datum, sealing land or socket geometry. In the modeled pose the camera envelope spans Z1..72 mm: it is 1 mm behind the bezel, 5.35 mm above the shelf, about 1.35 mm from the original front cradle vertically, 2 mm from the widened side arms, and 2 mm ahead of the rear keeper. These clearances require measured housing-safe packing/contact adjustment. A collision-free envelope does not establish a rigidly retained or flush optical face.

## Access and future expansion

The removable front wall exposes the lower board region without removing the PCB. Confirm practical finger/tool and USB-plug access with the actual board and cables. Coarse thumb screws are accessible from the panel side.

The open carrier and tray aperture reserve a route toward J11. Future RAW expansion uses a replacement rear cover/module. No DCA1000 board, mating adapter, ribbon routing or complete RAW electronics package is validated inside this enclosure. GPS and other unspecified extras are also not fit-validated.

## Printing and physical release gates

- Print the integral membrane face flat on the bed; support the offset broader wall regions without placing support texture against the membrane. Do not bridge the 52 mm membrane opening unsupported.
- The integrated main body needs provider-reviewed supports beneath PCB posts, tray and other overhangs. Support removal must not damage the 1 mm walls or mounting lands.
- Test the common coarse screw and the integral tripod thread. Coupons should reflect the actual print orientation; the tripod thread is horizontal in a front-face-oriented body print.
- Verify camera retention/optical and microphone exposure, connector routing, heatsink clearance, sealing, pin fit, thermal behavior and tripod load/creep before installing valuable electronics or outdoor operation.

Conclusion: the checked nominal geometry is collision-free and materially more complete than a bare-board fit study. It remains a fit-check prototype, not a production-fit, waterproof, RF, thermal or load certification.

Sources: TI Rev G design package, https://www.ti.com/lit/zip/sprr418 ; official STEP discussion, https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1333390/iwr6843aopevm-cad-files ; Logitech C920e datasheet, https://www.logitech.com/content/dam/logitech/en/video-collaboration/pdf/c920e-datasheet.pdf

## Final mesh cleanup

After the collision checks, the carrier-support front planes were moved from Z3.0 to Z3.2 mm, with their rear planes remaining at Z40 mm. This removes material only and resolves two tangent STL edges near X±66..67 mm, Y−75 mm, Z3 mm; it cannot introduce a new solid-volume collision. The designer's final rebuild reports all 11 exported B-reps valid and single-solid, and all 11 STL files closed and edge-manifold with zero bad edges. The large component collision checks were not repeated for this subtractive-only cleanup.
