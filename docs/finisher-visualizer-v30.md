# Finisher Visualizer v30

This build enables the preview on 16 audited retail executables: 1.0, 1.1,
3.0, 3.1, 5.1, 6.1–6.4, 6.66, Rev 1, Rev 1.1, Rev 2, Rev 2.2, Rev 3 and
Rev 3.2. The six remaining LAB Plus patches are 2.0, 2.1, 4.0, 4.1, 5.0 and
6.0. Their executable identities were absent from the two supplied folders,
including recursive scans of renamed executables. DLC enemy validation is
deferred as requested.

`sync_patch_ports.h` supplies per-build field, view manager, virtual getter,
cvar and instruction fingerprints. `sync_patch_layout.h` selects a descriptor
by PE identity. The equipment sentinel and momentum flag follow the selected
patch. The 6.x selector independently checks priority followed by the nearest
animation start, with the earlier candidate winning an exact tie.

The `--finisher-only` mode in `patch_port_tests.cpp` maps the local EXE as data
and runs the production code and reflection guards, including a corrupted
instruction check in the private image. All 16 EXEs passed. To reproduce the
audit with a compiled `PatchPortTests.exe`, pass it to
`Tools/PatchAudit/audit_executables.py` with `--probe` and
`--probe-arg=--finisher-only`.

A read-only check on running 3.1 identified Carcass, 63 shot spheres and
13 authored Glory Kill regions with known priorities. Removing Seek and
Destroy restored a valid range reading. Both modes produced the enemy outline
in observer replay. This check did not exercise an actual finisher or verify
all game overlays visually. The native regression suites and launcher and
WebView checks cover existing behavior separately.
