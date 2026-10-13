# R02 simplified radar/camera enclosure — prototype print handoff

R02 supersedes R01. Do not print R01: its provisional radar-window location was superseded by the actual Rev G antenna coordinates. Units are millimetres. This package is a fit-test prototype, not certified waterproof, structurally rated or thermally/RF validated.

## Inventory and size
Print one each of parts 01–05, twelve copies of 06 (the common custom thumb screw), and two copies of 07 (PCB pins): five main parts, nineteen installed printed pieces, seven unique types. C01–C04 are optional fit coupons, not installed parts. No newly purchased metal hardware is specified. Existing tripod hardware remains external. Existing TI bracket is not integrated.

Body envelope: 170 W × 210 H × 125 D. Assembled envelope including screw heads: 170 × 210 × 142 (front Z−7, rear Z135). Main skin is 1 mm; structural base, ribs, posts and sealing lands are thicker. The nominal 1/4-20 female tripod thread is integral to the reinforced base, approximately 14 mm modeled depth. It uses an approximate printable flank profile, not a certified Unified thread. Match the actual screw protrusion, test gently, and load-test an empty enclosure before installing electronics. Printed threads can wear, strip and creep.

## Print review before ordering
All solids and exported meshes are checked separately. Geometric validity does not establish print quality. Ask the provider to review support removal and handling of 1 mm skins.
- Main body: the integrated PCB posts, electronics tray, rear bosses and projecting camera bezel need support. Support must be removable through the front/rear openings; soluble support may be appropriate. Do not assume a simple unsupported front-on-bed print.
- Front service wall: place the flat 0.30 mm membrane face on the bed, with supports under the offset surrounding wall. Do not attempt a 52 mm unsupported thin bridge. Keep the RF membrane homogeneous, fully solid, untextured and without supports against its exposed surface. A 0.10 mm layer process gives three layers, subject to printer qualification.
- Rear cover and clamp: orient broad faces for stable support; inspect the split cable channels and sealing edges.
- Common screws and PCB pins: print fit samples first. The screw is a custom nominal Ø6 / 2 mm pitch part, not interchangeable with standard M6 hardware.
- Tripod coupons: test the clearance variants in the SAME thread-axis/build orientation as the body. Default body radial clearance is 0.25 mm. A vertical coupon does not qualify a horizontally printed socket.

## Assembly and service
1. Dry-fit all threads, covers, camera envelope and PCB mounting pins before sealing. Remove supports without stressing the thin membrane or PCB posts.
2. Fit the PCB to the two official Ø2.54 mounting-hole coordinates using the two printed pins. The outward AOP face points toward the thin membrane. Never press the chip against it. The nominal CAD face gap is about 2.935 mm; verify physically.
3. Seat the C920e in the integrated cradle and fit compliant housing pads/shims, then the keeper bar with two common screws. Published 94 × 43.3 × 71 dimensions already include the clip. The modeled maximum envelope clears the case but does not locate the actual lens glass or clip articulation. Nominal cradle side gaps are 3 mm; rear arm side gaps 2 mm; keeper-to-envelope rear gap 2 mm; shelf-to-envelope gap 5.35 mm. Pad the housing, not the lens. Camera front opening is 90 × 38 and needs physical checks for lens/mic clearance and a custom perimeter seal.
4. Front service wall uses four common screws. Removing it exposes the outward board face, mode switches and reset area without removing the PCB. Rear cover removal accesses opposite-face connectors and cable routing. Disconnect power before handling electronics.
5. Route attached USB cable connectors through the open rear edge, then close the split cable clamp with two common screws. Nominal cable passages are Ø6; measure actual cables, provide compliant seals and strain relief. The rear cover uses four common screws. Do not leave unused channels open in wet conditions.
6. Apply compatible fully cured sealant or fitted soft seals on perimeter lands, away from optics, switches and the RF aperture. Keep the rear/front covers serviceable. Test for leaks without electronics. No open drain is modeled; condensation, venting and cable seals require a deliberate physical test plan.

## Scope of the fit check
The official TI PROC091G STEP includes more than the AOP board. A spatially classified 1,437-solid AOP reference was extracted and placed using a proper 180° Y rotation. This is a derived reference, not an independently named vendor subassembly. Its modeled components, five core printed parts and installed screw geometry were checked for intersections. Logitech's full unrounded published including-clip box was checked separately. The checks do not establish real camera pose, heatsink or bracket clearance, connected-cable bending, fastener strength, printer tolerance, seal compression, or assembly access under every configuration.

The actual PCB outline is 115.26791 × 25.27145 from the fabrication CAD; nominal drawing dimensions are 115.01 × 25.02 × 1.62. STEP board thickness is approximately 1.604 mm. The two mounting holes are about 35.8237 mm apart. Do not substitute the unrelated 22.72 mm MCH055 stand drawing.

The installed heatsink has not been established in the classified model; verify its complete position and envelope before printing. GPS module/antenna and extra electronics are unspecified: the integrated tray is provision only, not a fit guarantee. Keep the GPS antenna away from metal and validate reception. The radar axis is rotated with respect to the source CAD; confirm firmware/world-axis mapping.

## Future RAW upgrade
Rev G uses J11 for direct DCA1000 connectivity. The default enclosure does not contain a validated DCA1000 stack. A future replacement rear cover can carry an external module and suitable cable/connector access. DCA1000's bare PCB is 90 × 102 mm, with additional Ethernet, power, USB and access needs; cable length/bend limits and a completed pod remain to be designed for the chosen hardware. J11 access is reserved through the tray opening.

## Environment and testing
PLA softening/creep, thin-window puncture, RF loss/detuning, camera water ingress and enclosure temperature are unresolved. The camera itself is not established as weatherproof. TI operating limits are not enclosure thermal certification. Perform dry-fit, optical view, mode-switch/USB service, RF comparison, temperature, leak and tripod proof-load tests before outdoor use. Avoid conductive paint or metal in the antenna beam. No electronic modifications are specified.

## Source and editable files
See THIRD_PARTY_REFERENCES.md for source links and exclusions. Third-party device geometry and reference-containing renders are not distributed in this public edition. cad/build_enclosure.py is the editable CadQuery source; cad/step/ and cad/stl/ contain individual parts. cad/assembly.step contains five main parts and twelve screws; it omits the two PCB pins. The optional enclosure-only preview script is tools/render_preview.py. Public-edition status and later camera-proxy caveats are in the root README.md.

Official sources:
- Logitech: https://www.logitech.com/content/dam/logitech/en/video-collaboration/pdf/c920e-datasheet.pdf
- TI Rev G design package: https://www.ti.com/lit/zip/sprr418
- TI engineer's STEP source: https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1333390/iwr6843aopevm-cad-files
- TI board/service guide: https://www.ti.com/lit/ug/swru546e/swru546e.pdf
- DCA1000 guide: https://www.ti.com/lit/ug/spruij4a/spruij4a.pdf
- TI radome guidance: https://www.ti.com/lit/an/swra705/swra705.pdf
