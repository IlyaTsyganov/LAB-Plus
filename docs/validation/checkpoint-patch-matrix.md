# Checkpoint save/load patch matrix — 2026-11-02

All 22 recognized retail executable builds now have statically audited save/load descriptors. Audits use exact SHA-256/PE identities, method fingerprints, the retail restart parameter constructor/caller, reflected system members, save-manager queue layouts and current-user slot records. `verify_practice_profiles.py` independently verifies the configured descriptors against the actual EXEs without launching a game. Original executable installations, downpatch resource directories and the user's descending overlay procedure were not modified.

Static verification and synthetic fixtures are separate from live gameplay acceptance. The [1.1 live test](checkpoint-live-1.1.md) returned from a later checkpoint B to archive A while game and Steam stayed running. Other rows below remain pending until actually exercised in game.

Further gameplay testing targets representative save-manager/loader layouts to reduce repeated manual work, rather than launching every patch. Untested individual builds retain their pending status; shared-layout coverage is not presented as a full gameplay pass. A read-only archive was also captured on 2.1, but its B-to-A cycle has not been performed.

## Campaign availability

- 1.0 through 2.1: main campaign, slots 1–3 (`GAME-AUTOSAVE0..2`).
- 3.0 through 4.1: main campaign plus TAG1, six slots (`DLC1-AUTOSAVE3..5`).
- 5.0 and later: main campaign plus TAG1 and TAG2, nine slots (`DLC2-AUTOSAVE6..8`).

The native reader rejects slots from unavailable campaigns. Managed import/restore eligibility uses the same boundaries. Settings restrict in-game slots to the detected running executable; the advanced offline file workflow remains available with the game and Steam closed. Archives remain bound to the same executable patch, Steam account and original slot.

Map-family validation includes the exact main-campaign Fortress map `game/hub/hub` and TAG1 hub `game/dlc/hub/hub` in their respective families. Native fixtures check Fortress acceptance for all three main slots on every descriptor, and reject the wrong campaign hub and misleading suffixes. This prevents a valid main-campaign hub save from being rejected by the level-map filter; live hub acceptance is separate from mission-checkpoint testing below.

| Patch | Available slots | Static EXE audit | Live acceptance in this session |
|---|---:|---|---|
| 1.0 | 3 | Passed | passed: main campaign, slot 2, B -> A |
| 1.1 | 3 | Passed | passed: main campaign, slot 2, B -> A |
| 2.0 | 3 | Passed | passed: main campaign, slot 2, B -> A |
| 2.1 | 3 | Passed | archive captured; B -> A not run |
| 3.0 | 6 | Passed | passed: TAG1, slot 1, B -> A |
| 3.1 | 6 | Passed | not run in this session |
| 4.0 | 6 | Passed | not run in this session |
| 4.1 | 6 | Passed | not run in this session |
| 5.0 | 9 | Passed | passed: TAG2, slot 1, B -> A |
| 5.1 | 9 | Passed | not run in this session |
| 6.0 | 9 | Passed | not run in this session |
| 6.1 | 9 | Passed | not run in this session |
| 6.2 | 9 | Passed | not run in this session |
| 6.3 | 9 | Passed | not run in this session |
| 6.4 | 9 | Passed | not run in this session |
| 6.66 | 9 | Passed | not run in this session |
| 6.66 Rev 1 | 9 | Passed | not run in this session |
| 6.66 Rev 1.1 | 9 | Passed | not run in this session |
| 6.66 Rev 2 | 9 | Passed | passed: main campaign, slot 2, B -> A |
| 6.66 Rev 2.2 | 9 | Passed | not run in this session |
| 6.66 Rev 3 | 9 | Passed | **Repeats passed without debugger; initial crash unexplained** |
| 6.66 Rev 3.2 | 9 | Passed | **Repeat passed; one initial unexplained crash** |

The source data and SHA-256 values are in `checkpoint-patch-matrix.json`. No game binaries or save containers are committed.

## Reproduce static verification

```powershell
python Tools/PatchAudit/verify_practice_profiles.py --out build/practice-audit.json "PATH/DOOMEternalx64vk.exe"
```

The Python verifier uses only the standard library and the repository's PE reader. Native practice fixtures exercise all nine slot choices against each descriptor (including rejected pre-DLC choices), loader fingerprint mutations, pause/save-queue checks and rollback. Managed tests check the 3.0/5.0 DLC boundaries and version identity in the restore protocol. Before live restore, the native callback verifies the running patch, current slot, pause/idle state, account, Cloud state and both Steam-cache and disk bytes, then retains originals before writing.

## Live 1.0 result

Main campaign slot 2, same process PID 1136 and Steam kept open. Archive A (`877964dfa39245048b654ed2a9021a28`) captured `cp_03_post_barge`; after the user progressed, archive B (`d665703d1f67481b8dc37382b784c40b`) captured `cp_04_pre_cathedral`. Installing A through Steam Storage completed with verified disk/cache bytes and an observed engine transition back to `cp_03_post_barge`. The user then reported the game paused. B originals are retained under `.labplus-live-original-0c6ac7e62f3b4a9389e7d33c435a237c`. Cloud was effectively off (account 0 / app 1). No other slot or PROFILE was targeted.

## Initial 3.0 observation (interpretation superseded below)

The initial read-only preflight on 3.0 / TAG1 observed `AUTOSAVE3` with numeric index 3, while the previous 6.66 Rev 3 adapter used campaign-relative indices (for example `AUTOSAVE5` with index 2). The initial strict index/folder check rejected the operation before archival or restore writes. The first interim mapping accepted the two observed numbering contracts while requiring the exact canonical folder, current map family, patch campaign availability and requested slot. The numeric field never constructs a file path or selects a transaction target. Synthetic fixtures cover both contracts for all 22 profiles, contradictory numbers/folders and the 3.0/5.0 availability boundaries. A repeated read-only probe on the same 3.0 process accepted `DLC1-AUTOSAVE3`, with all loader signatures matching. In-game A/B acceptance requires the game to load the updated DLL once.

## Live 3.0 / TAG1 result

After loading the slot-numbering fix once, TAG1 slot 1 (`DLC1-AUTOSAVE3`, global record index 3) passed the complete B-to-A cycle. Archive A (`4f16d750af684e99aa10c611722add9f`) captured `01a_inside_building_a`; archive B (`65ac076c2e4749ebbcc898dfd1bc8fec`) captured `01b_post_yellow_key`. Restoring A through Steam Storage verified disk/cache bytes and completed an engine transition back to `01a_inside_building_a`, in the same process PID 18520. The user confirmed the return worked and paused the game. B originals are retained under `.labplus-live-original-2d335861577640249b53e8d099319893`. Steam and LAB Plus stayed running, and Cloud remained effectively off.

## Live 5.0 / TAG2 result

TAG2 slot 1 (`DLC2-AUTOSAVE6`) passed the complete B-to-A cycle. Archive A (`9ed0ceef9ce04993b9930eb197b1a3f3`) captured `cp_02_screecher_intro`; archive B (`d4357a13ed8b4fd9a8359f480f2632f8`) captured `cp_03_village_entrance_locked`. Installing A verified Steam-cache and disk bytes and completed the engine transition back to `cp_02_screecher_intro`, in the same process PID 13528. The user confirmed rollback worked and paused. B originals are retained under `.labplus-live-original-3498aac52429445f82c5693ad8fe0bcb`. Steam and LAB Plus remained open, with effective Cloud off.

## 6.66 Rev 3.2 live failure

Main campaign slot 2: A (`88fe4d201350466fadb1c8f9a21604ae`) captured `cp_04_pre_cathedral`; B (`59b9879338314363b1d51a8ef6214a5d`) captured `cp_04a_upper_cathedral`. Steam installation and byte verification succeeded, and the bridge observed a transition to A, but the game then exited after the user pressed Space as the level appeared. This is **not** a passed loading test. The current completion criterion can report too early and needs strengthening. B originals are retained under `.labplus-live-original-c67a792e11374e0aa8e8951464b0cfef`; both archives remain available. The native log and response were preserved locally before another launch. No further archive replacement is being attempted until the fault is diagnosed.

## 6.66 Rev 3.2 diagnostic repeat

The original crash did not reproduce under CDB. Normal menu loading succeeded, an engine-only reload with no file replacement succeeded, then a complete fresh B2-to-A2 cycle succeeded. A2 (`df575f49f85d4be881877f25ac57a119`) captured `cp_04_pre_cathedral`; B2 (`28e03139cf9c4c8990a7d67ed60f889a`) captured `cp_05_post_cathedral`. Restoring A2 completed back to `cp_04_pre_cathedral` in PID 24592, and the user confirmed it worked. Originals are retained under `.labplus-live-original-e08a7e11b820455688fc51c8db61847c`. The initial crash cause remains unknown; this evidence does not establish that it was unrelated to LAB Plus.

The completion guard was strengthened after this observation: a map transition alone is insufficient. It must observe a real active-player gameplay frame after the loading prompt, then a continuously verified session for two seconds (allowing the user to pause again). Tests cover the formerly premature map-only result, waiting before Space, interrupted readiness and reset on another transition. The updated DLL is included in the local all-patches test package for subsequent launches.

## Corrected discovered-entry semantics

Live 6.66 Rev 3 / TAG1 slot 3 exposed the underlying issue: the active record reported folder `AUTOSAVE5`, index 3 and list count 4. Retail `GetCurrentSlotInfo` indexes a per-user array at object+0x80, count at object+0x88, stride 0xe0. This is an index into discovered save entries, not a global or campaign-relative physical slot number. Static disassembly confirmed this layout for all 22 builds; getter RVAs are recorded in the matrix JSON. The selected entry's folder string (+0x78 within the entry) was independently read and matched `AUTOSAVE5`.

The interim numeric-to-folder mapping has been removed. The reader now bounds the discovered-entry index, validates the selected entry's exact name against the active record, resolves the canonical physical folder, enforces patch campaign availability and map family, and requires an exact match with the requested transaction slot. Fixtures include sparse lists where `AUTOSAVE5` is entry 3 of 4, mismatched entry/folder rejection, out-of-range counts/indices and independently arranged list positions for all 22 builds. The canonical physical folder-to-Campaign/TAG1/TAG2 slot table never changed.

## 6.66 Rev 3 live failure

TAG1 slot 3 (`DLC1-AUTOSAVE5`), PID 24868: A (`e520b3b2c6fd4abab8be9ce2b83f3e87`) captured `cp_01a`; B (`ddb66f8b5bf545e7ab29e0253a838746`) captured `01a_inside_building_a`. The archive installation passed Steam-cache and disk verification, but the game exited after Space as the level appeared. The strengthened bridge remained in `loading` and did not report success. B originals are retained under `.labplus-live-original-f68e8d45e6614d6183ce36fe0ae4f9d6`; both archives and the response/native log are preserved locally. This second observation means the earlier Rev 3.2 crash cannot be dismissed as unrelated. Diagnosis continues with normal menu loading and engine message capture without a debugger attached.

## 6.66 Rev 3 diagnostic repeats without debugger

In a fresh process PID 24116, ordinary menu loading of the installed A succeeded. An engine-only reload with no replacement succeeded. Loading the existing B archive and returning to A both succeeded. Finally the user naturally progressed again to `01a_inside_building_a`; a fresh B2 (`43ba80d1df0a4fdd9c897c29f4a4260c`) was archived, and B2-to-A succeeded with originals retained under `.labplus-live-original-be4c34ebeead440dbf2335ec69f5d70c`. The user confirmed each load and paused afterward. The loader code was unchanged during these repeats. The diagnostic helper collected OutputDebugString messages and watched process lifetime without attaching a debugger; successful-repeat logs and final bridge response are retained privately. These passes do not explain or fix the initial crashes. Message/exit capture remains enabled during subsequent patch tests.

## Live 2.0 result

Main campaign slot 2, PID 22872: A (`11599b6284f347519fb1fe36d32889f1`) captured `cp_04_pre_cathedral`; B (`5b00a792982e4969b9f6ba6de074601a`) captured `cp_05_post_cathedral`. Restoring A passed Steam/disk byte verification and the transition plus active-player stability checks. The user confirmed rollback without a crash. Originals are retained under `.labplus-live-original-8620368fc985410fbd3ae6748a445519`. Steam and LAB Plus stayed running, with effective Cloud off; the message/exit listener remained attached to named debug-output events only, without a debugger.

## Automated verification after the diagnostic changes

The static verifier passed against all 22 distinct retail builds, including the discovered-entry accessor layout extension. The native layer and practice test executable rebuilt successfully; native practice and managed launcher suites passed. The headless UI suite passed, including interrupted-load error presentation. The real WebView2 saves-only workflow passed with isolated synthetic data in the user's Windows session: offline archive/restore, exact ciphertext return, restore review, 3/6/9 in-game slot choices, and 125%/150% scaling. Its first attempt from the isolated execution account failed to create a WebView2 controller; the user-session repeat passed. No user save was touched by these automated fixtures.

The managed bridge now preserves the original checkpoint recovery location when the game closes or the selected process changes during restore. The UI reports interrupted loading instead of suggesting Space in an exited game. Integration fixtures exercise the asynchronous production mailbox after `loading` and an already-exited child process. This improves failure reporting and recovery, but does not establish the cause of the observed game crashes.

## Live 6.66 Rev 2 result

Main campaign slot 2, PID 22484, with build identity independently confirmed by the native response: A (`b84d7ec94f2445299ec09cf3da30e719`) captured `cp_04_pre_cathedral`; B (`0af86c826393489c8c4e07757ee62bfc`) captured `cp_05_post_cathedral`. Installing A passed Steam/disk byte verification and the observed transition plus active-player stability checks. The user confirmed the return worked without a crash. B originals are retained under `.labplus-live-original-4f6927e06411402f8d52c63a331532d0`. Game, Steam and LAB Plus stayed running with effective Cloud off and without a debugger attached. The private message/exit capture was active.

This completes the reduced manual series, including the shared-task save-manager layout introduced by Rev 2. Clean B-to-A cycles passed on 1.0, 1.1, 2.0, 3.0/TAG1, 5.0/TAG2 and 6.66 Rev 2. Rev 3/TAG1 and Rev 3.2/main campaign had successful repeats but also one initial crash each, and remain under investigation. Remaining individual builds retain static/synthetic coverage only; all-patch live stability is not claimed. No release, remote commit or tag was published.
