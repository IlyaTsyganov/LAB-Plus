# Finisher Visualizer v31

The preview now has a profile for every retail build recognized by LAB Plus.
The new profiles cover 2.0, 2.1, 4.0, 4.1, 5.0 and 6.0. The executables in
Downpatcher match the SHA-256 identities from the original patch audit.

`Native/CollisionViewer/src/sync_patch_ports.h` records each build's reflected
field changes and the addresses used to read target geometry and range data.
It also records instruction fingerprints for ordinary Glory Kill and Chainsaw
selection. The profiles for 4.x and 5.0 account for changes in the path from
selection to execution. Each build uses priority followed by the closest
animation start, with exact ties retaining the earlier candidate.

`Native/CollisionViewer/tests/sync_patch_checks.h` checks that every recognized
game build has a Finisher Visualizer layout and that a modified executable
identity is rejected. `PatchPortTests --finisher-only` checks the production
decoder, reflected layouts and instruction guards on each of the 22 local
EXEs mapped as data. The native regression suites exercise the existing
selection, geometry and HUD behavior separately.

The six new profiles were checked offline. Their in-game overlays have not
been visually tested. DLC enemy validation remains deferred. For a live test,
restart both LAB Plus and the game, launch the chosen patch from v31 and use
Glory Kill preview with Seek and Destroy unequipped.
