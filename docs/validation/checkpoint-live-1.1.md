# DOOM Eternal 1.1 in-game checkpoints — 2026-11-02

## Scope and implementation

A separate save/load adapter now covers patch 1.1, in addition to the existing 6.66 Rev 3 adapter. Collision support for 22 builds does not imply save/load support for those builds. Unknown save ABIs remain unavailable; the offline file workflow is retained.

The patch adapter selects the engine globals, pending-save count, save-manager layout, current-user slot records and loader/string methods. In 1.1 the pending request count is at system+0x9c38, the save manager at system+0x9b10, the writer list count at ghost+0x40 and the slot list at ghost+0x68. Read-only checks in the running retail process confirmed these fields, the current slot, pause and all five loader method fingerprints. Reload follows the retail restart parameter construction, selecting PRIMARY containers.

The bridge now carries the actual canonical slot and executable patch through a V2 response and restore plan. Restore requires the archive's original slot, Steam owner and patch. Steam API keys and on-disk transaction paths use that selected slot. A mismatched patch is rejected before writes. Archives retain all four authenticated containers. Writes occur in the engine frame callback only after pause, idle, owner, Cloud, fingerprint and disk/cache checks. Current originals are retained before writing; failures during installation roll back all four files. Engine completion requires an observed map transition, rather than a queued request alone.

## Live acceptance

The user launched the new local build once to load its updated DLL, then kept the game and Steam running throughout these operations. Same game process throughout: PID 20136. Patch 1.1, main campaign, slot 2 (`GAME-AUTOSAVE1`). Effective Steam Cloud off: account flag 0, app flag 1.

1. Archive A captured from `game/sp/e1m1_intro/e1m1_intro`, loaded checkpoint `cp_03_post_barge`. All four files authenticated and disk/Steam bytes agreed. Archive ID: `a03de137ecd645f98c1aaa7a6e6b58ca`.
2. Engine-only primary reload completed back to `cp_03_post_barge`, with no files replaced.
3. Full archive A install through Steam Storage completed, disk/cache verification passed and the engine returned to `cp_03_post_barge`. The user confirmed normal loading without a crash.
4. The user progressed to `cp_04_pre_cathedral`, then paused. Archive B captured and verified separately: `095c8ffac9ca4bcebc3b73113968744a`.
5. At the user's explicit request, archive A was installed again. The engine reported a completed transition back to `cp_03_post_barge`. The user confirmed: “да, все работает”. This demonstrates return to an older checkpoint, rather than reloading an unchanged current checkpoint.

The pre-replacement B files are also retained under the Steam app root in `.labplus-live-original-b4c8f707327748e7b23b259d146242b0`. The operation journal is under the LAB Plus data directory in `live-practice/restore-pending/20136/b4c8f707327748e7b23b259d146242b0/RESTORE_PATHS.txt`. Neither PROFILE nor another slot is targeted by the live transaction. These are local recovery artifacts, not bundled game data.

## Automated verification

- Production native practice fixtures pass, including independent cryptography, transaction rollback, changed target/cache/idle rejection, the 1.1 main-campaign adapter, 1.1 pending/writer guards and changed loader fingerprint rejection.
- Managed launcher fixtures pass, including V1 archive compatibility, V2 main-slot/patch identity, native-plan patch binding and modified/unsupported patch rejection. Offline nine-slot tests still pass.
- InspectorPreview fixtures pass, including the production live protocol and the offline default workflow.
- Local native and managed Release builds succeed. No GitHub push, tag or release publication was requested or performed.

## Limits at the time of this test

The subsequent [all-patch matrix](checkpoint-patch-matrix.md) supersedes the adapter coverage limits below.

This is a completed-checkpoint archive workflow, not an arbitrary RAM save state. The 1.1 acceptance above covers main-campaign slot 2 on one map. Other 1.1 slots share the adapter and canonical mapping but did not receive live gameplay acceptance. 6.66 Rev 3 retains its audited adapter; the newly generalized slots require their own acceptance. Other recognized game patches are not enabled for in-game save/load until their ABIs are audited. Save files are not converted between patches or accounts.

Final verification also passed the native parser against the actual managed 1.1 restore bundle, including rejection of a mismatched patch, and the focused browser Save/Load/settings scenario at 125% and 150% scale. The final local 30-file package is under artifacts/live-saves-win-x64; the running installation was left in place.
