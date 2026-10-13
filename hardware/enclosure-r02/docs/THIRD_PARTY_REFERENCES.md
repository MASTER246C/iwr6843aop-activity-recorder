# Provenance and excluded third-party materials

## Published material

This public edition contains the authored R02 enclosure generator, its part/assembly exports, configuration, original and refreshed validation reports, and enclosure-only documentation/preview. It does not contain third-party device meshes, vendor downloads, electronic firmware, private sensor/camera recordings, a web viewer, paid shaders, or binary Blender scenes. Dimensions and source links are retained as engineering context, without implying manufacturer endorsement.

## C920 community model: linked, not redistributed

- Community repository: https://github.com/sci-bots/logitech-c920
- Locally preserved source attribution: Ahmet Taskiran / GrabCAD.
- The repository was recorded as MIT-licensed; its saved license says Copyright (c) 2017 sci-bots. That alone does not establish redistribution rights in the earlier GrabCAD model.
- Original STEP SHA-256, recorded from the source artifact: `3c77102981878c47cb632e5515f37cff7fade0218ecfcea1ac3d11a61c2e1da6`.
- No original or transformed C920 STEP/STL, preview derived from that model, bundled model archive, or associated viewer asset is included. Anyone obtaining a model separately must verify its applicable rights.

The model is a community C920 proxy, not a manufacturer-certified C920e. Its prior intersection-check results are historical observations only. The 5.6117709519 mm keeper gap and 0.7500000502 mm shell clearance refer to that particular reference pose, not measured hardware.

## TI model: linked, not redistributed

The earlier engineering study used a spatially classified subset of TI PROC091G STEP geometry. Distribution rights in the vendor model have not been established for this public edition, so the original/derived STEP and STL, PCB-outline trace, device-containing render and Blender scenes are excluded. The enclosure generator is self-contained and does not read that model.

Official references:
- TI Rev G design package: https://www.ti.com/lit/zip/sprr418
- TI STEP discussion: https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1333390/iwr6843aopevm-cad-files
- TI board/service guide: https://www.ti.com/lit/ug/swru546e/swru546e.pdf
- TI radome guidance: https://www.ti.com/lit/an/swra705/swra705.pdf
- DCA1000 guide: https://www.ti.com/lit/ug/spruij4a/spruij4a.pdf
- Logitech nominal dimensions: https://www.logitech.com/content/dam/logitech/en/video-collaboration/pdf/c920e-datasheet.pdf

## Dependencies

CadQuery is required to rebuild the geometry, and Matplotlib/NumPy to reproduce the optional preview. They are not vendored and retain their own licenses. The existing Three.js viewer is outside this CAD publication package because its current asset set includes the withheld device references and internal delivery metadata. No rights to third-party trademarks or materials are granted by the enclosure license.
