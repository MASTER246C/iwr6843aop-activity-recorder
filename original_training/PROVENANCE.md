# Provenance and package boundary

Tool: Original Radar Host Training Tools 0.1.0-preview.2.
Schema versions remain 1; source protocol is original-radar-v2.

This is newly authored local Python standard-library code and documentation for
original protocol session exports. It is separate from the pre-existing TI-only
JavaScript ML implementation. No existing shared module or firmware binary is
modified or copied into this package. The candidate viewer is the dataset producer;
it is not bundled and is not a runtime dependency once exports exist.

LICENSE is copied byte-for-byte from Radar Studio's Source-Available No-Resale
license. UPSTREAM_LICENSE_COVERAGE.md preserves the upstream notice context; its
viewer inventory does not describe this smaller package. TI software/SDK/firmware
and documentation remain separately licensed and are not included. Original code
here is covered by LICENSE. Participant/dataset rights are separate and no private
recordings or trained weights are included.

The ZIP is a host analysis tool, not flashable firmware. It does not transmit RF,
open serial ports, communicate online or alter a board. Public package manifests
list only tool source, tests and documentation; synthetic tests create temporary
files locally and clearly mark all generated sessions as synthetic.
