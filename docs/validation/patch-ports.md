# Patch adaptation preview: 0.8.16

Checked 2026-11-01 against the ten user-installed Windows x64 executables. Base source: LAB Plus 0.8.12, remote main `bc3e4e47f00f685984beaf8c2765c88fa683413c`. This is a local test build; no new public release has been requested.

## Implemented

- Added per-executable observation code profiles for clock callers, stagger getters, status-effect lifetime override and animation event registry. All code is inspected/read; no engine getters are invoked.
- Preserved the original 54-byte clock getter and added the 47-byte compact form used by all seven 6.66 builds. Both read domain 1 at 999960 ticks/second. Caller displacement, map identity, scaling branch and instruction shapes are validated.
- Supported the 176-byte health component and `max.currentValue` from reflected `idBranchedOptionValue < declInputBranch_t , float >`. This is the engine-selected cached branch, distinct from `defaultValue`. The original 88/96-byte layouts and bounded health ancestry remain intact. Unknown/nonfinite maximum does not become a guessed bar ratio.
- Added exact BATS calculation/helper fingerprints for all seven 6.66 builds. Their 516-byte calculation and shorter Dexterity helper have independently checked constant/field operands; legacy 529-byte calculations retain their original defaults. Rate comparison, inventory ownership, current mode, animation clips and neutral-status checks remain required.
- Enabled data-validated Praetor player-release timelines on the ten audited event profiles. This still requires the exact asset, actual per-model release event, player ownership, matching registry definition, and supported animation clock. Other syncs retain their current-clip semantics.
- Added a reusable optional EXE audit tool and a native data-only probe for later installed versions. Unknown executables do not inherit another build’s code profile.

## Verification completed

- All 23 native regression suites passed on the final source, including expanded enemy-health and clock fixtures for all ten targets. Branch-selected maximum differs from default; changed branch values, unknown maxima, stale slots, deaths, malformed field sizes, wrong clock domain, changed code and foreign profiles are covered.
- Launcher fixtures passed; 294 managed HUD checks passed. .NET 10 Windows x64 publish succeeded.
- All ten real EXEs passed `RetailPatchPortProbe`: production PE identity, real reflection including `idAIHealth` ancestry, health stride/maximum offset, stagger getter pair, complete BATS hashes/constants/reflected operands, real clock instructions with synthetic scaled/unscaled clock data, lifetime override, sync ownership metadata, event getter/definition and corruption rejection. EXEs were copied into private nonexecutable memory solely for inspection; no game process, game executable or save file was modified.
- Runtime package validated: 29 files; `LABPlus.exe` FileVersion 0.8.13.0; packaged native DLL equals the tested DLL.

| Executable | PE timestamp | Image size | EXE SHA-256 | Offline audit |
| --- | --- | --- | --- | --- |
| DOOMEternal 6.66 Rev 3.2 | 0x6a7b9b8c | 0x7431000 | `69dc13e88d1c19133ead7950dc64ebcbd4a5a3f6bd6f9c336ebffe56df6a1c11` | passed |
| DOOMEternal 1.0 (Release) | 0x5e794131 | 0x1e706000 | `71aacc8f2185a5ffd54dc49b2196ca366aaac3cdc509fb32d6a9219268c9ac88` | passed |
| DOOMEternal 1.1 | 0x5ec7deca | 0x1d551000 | `0fa3177e7f74a5fe7571e1f2954db649dc99a1c8c6f6050a2a39ef799d37ea7f` | passed |
| DOOMEternal 5.1 | 0x607e69b0 | 0x1b5dd000 | `93da12b964438b8bcabeb8fb12ff4050cbc6042f723b00baa89544482292594c` | passed |
| DOOMEternal 6.66 | 0x61692c83 | 0x1e4cd000 | `4a6c7962562462e83b99d490412c23a6054972df15ff3f09f05c2193cbe2874f` | passed |
| DOOMEternal 6.66 Rev 1 | 0x618dfc01 | 0x1c835000 | `cb1f3a25c34ddee1878fb206c0ddb7508f6d0ddad4db272bd58bb8cacf93075f` | passed |
| DOOMEternal 6.66 Rev 1.1 | 0x61d7704e | 0x1c58a000 | `c7cf6702b5602a25f0969f748b72849e674b4f0c4cb898b102b6169d5fc4d26d` | passed |
| DOOMEternal 6.66 Rev 2 | 0x6256614c | 0x1e69d000 | `84d6d3d01855fa5900f7ff8e452a14d9316ebc459bafdc4c4480831b537ff375` | passed |
| DOOMEternal 6.66 Rev 2.2 | 0x64cc8a9b | 0x7421000 | `7352dc4da0b107939c7d4d1e2c42ea087c566c26e79f8e01270678a0c61f6b39` | passed |
| DOOMEternal 6.66 Rev 3 | 0x669f0dcf | 0x744d000 | `005f887a04450ab3f34535be658a711a8fcd3fbb602d07bec19b54ecbf53efa8` | passed |

## Live acceptance still required

No game was launched for this porting session. Offline audits and synthetic fixtures do not establish live behavior. Confirm each feature independently: enemy HP damage/death/despawn; bars and GK window; frag/ice/flame and both Sticky timers; powerups; mantle; BATS positive/negative weapon matrix; Fast DBlade preparation/holster/consumption; interaction, glory-kill and chainsaw timelines. Include pause, restart, level changes and application shutdown. Begin with 6.66 Rev 3.2.

The standard Meathook `XINPUT1_3.dll` is present in the installed 1.0, 1.1, 5.1 and Rev 3.2 directories. All four copies are identical (439296 bytes, SHA-256 `02c7159964245e249bcffc12598651b871f6c73925c4e04355510587114cdab3`) and export `mh_api_get`. It is absent from the other six game directories. Those installations need a working compatible Meathook before metadata-based observations can be tested live. No DLLs were installed into game directories.

The existing lighting GPU/version policy, firewall launch requirements, UI, Wiki and user settings were not changed. The old release-request file is intentionally unchanged; this preview is not a request to publish a new release.

## Repeating the EXE audit

Build the native project normally (`build-viewer.cmd` or `.ps1`). The optional probe is named `RetailPatchPortProbe.exe`, so normal `*Tests.exe` discovery does not try to run it without retail inputs. Python 3 is required for inventory; numpy is optional for faster indexing.

```powershell
python Tools/PatchAudit/audit_executables.py --out build/patch-audit --probe Native/CollisionViewer/bin/RetailPatchPortProbe.exe "PATH/DOOMEternalx64vk.exe"
```

The tool writes EXE hashes, PE identities, candidate metadata RVAs, probe logs and `results.json`. It stores no instruction arrays or EXE copies. Keep generated metadata/RVA inventories and retail diagnostics local; they are not user documentation or a substitute for a live acceptance record. A `passed` result means the offline production audit passed, not that gameplay was demonstrated.

## Local package

Executable: `C:/workspace/LABPlus-0.8.13-patch-preview/LABPlus.exe`.
ZIP SHA-256: `fa861a77a5bdb9962c537ee7c10e6eabb97b0d0a3a2e30a08a884687e000579f`.

## 0.8.14: Praetor Token event format correction

The 0.8.13 offline audit did not exercise the full interaction-to-event path. The user's Rev 3.2 live pickup had no timer. A read-only metadata check confirmed that 1.0/1.1 use `idAtomicStringT < md6AnimAtomicString_t >` plus a stored `disabled` bool, while 5.1 and all seven 6.66 builds use `idMD6AnimRef` (a reflected subclass of `idAtomicString`) and encode disabled state as `ANIM_EVENT_FLAG_DISABLED = 2` in `animEventFlags_t`.

The corrected reader verifies the reference's actual ancestry and string field, and validates the exact executable's enum definition and all five flag names/values before accepting packed event storage. Missing or malformed bool fields are not silently treated as enabled. The authored release frame, selected player model, time domain, animation rate, pause time and ownership checks remain mandatory.

`SyncInteractionTests` now exercise both event formats on all ten profiles, including disabled state, changed enum values, unrelated same-sized references and unknown EXEs. `RetailPatchPortProbe` now constructs synthetic interactions with **each EXE's real reflected layouts**, runs `captureSyncAnimationTimers` through to `timerHudLines`, and validates positive release timing, the disabled guard, pause/clock behavior and removal after player ownership ends. This full-path audit passed on all ten installed EXEs. Its 94/30-second fixture is a synthetic input, not a claim about every retail asset's authored duration.

Targeted SyncInteraction, GameTimer and InvisibleNumbers regression tests passed. The 0.8.14 runtime layer and application were rebuilt. The game was in the loading/menu state during read-only inspection, so a new positive live pickup with the corrected DLL remains required. No game process memory, input, saves or installation files were changed. Fully restart the game to load the corrected layer.

## 0.8.15: live interaction diagnosis and general release boundaries

A real Rev 3.2 recording showed the remaining failure: `ae_releasePlayerFromSync` has **one** implicit `idMD6Anim* anim` argument (format `4`), while the old resolver and its synthetic registry fixture incorrectly required zero. The event registry returned index715, kind2, argument count1, types `idMD6Anim*;`, names `anim;`. The declaration constructors in all ten installed EXEs independently confirm this same signature. The reader now verifies the complete signature. The offline audit additionally verifies the actual retail declaration constructor's normalized fingerprint and its string operands, so a synthetic registry cannot conceal another signature mismatch.

Player-release timing now applies to all player-owned interaction and non-melee scripted syncs on the audited profiles. There is no Praetor asset-name whitelist. The current NPC clip is not substituted for an unknown player-control deadline. Melee/glory-kill clip timers retain their existing semantics.

Live evidence on 6.66 Rev 3.2, with all game actions performed by the user:

| Scenario | Evidence | Result |
| --- | --- | --- |
| Praetor Token, `game/sp/e2m1_nest/e2m1_nest` | Corrected production reader, release event715 at frame94/30fps, 12 positive samples and ownership release | Maximum3.13333s; valid descending countdown; inactive while the NPC animation continues |
| Golden Gore Nest, same map | Corrected production reader, event715 at frame65/30fps, 9 positive samples and ownership release | Maximum2.16667s; valid descending countdown; inactive after player release |
| Ordinary Gore Nest, `game/sp/e1m3_cult/e1m3_cult` | Initial read-only diagnosis recorded a233-frame NPC clip and a release event at81/30fps; ownership samples bracket the release deadline | Correct control duration2.7s; old clip duration7.73333s was the wrong boundary |

These observations confirm the corrected **reader** on the running game. The observer uses VM_READ only; it does not inject, control the game or replace the currently loaded0.8.14 DLL. Visual confirmation of the newly packaged HUD requires a full game restart with0.8.15.

The full retail audit passed on all ten EXEs with real reflection, real event declaration constructors, and synthetic Praetor, ordinary Gore Nest, Golden Gore Nest, arbitrary-name and script-started interactions. Fixtures verify per-asset release frames rather than fixed durations, plus disabled events, pause, wrong signatures, foreign types, generation changes and owner release. All23 native regression suites passed after updating the old clip-only timer fixture to require unavailable state when release metadata is absent. No Wiki, game installations, save files or user preferences were changed.

Private diagnostic traces are in the task workspace's `sync-debug/` directory and must not be included in source publication. `tests/sync_live_probe.cpp` is the bounded read-only observer source. It accepts `PID BASE metadata.tsv output.log seconds`, with a300-second maximum. Metadata RVAs must be rebased to the current process and the TSV header must name the current PID. Synthetic replay and retail audits remain separate from live gameplay evidence.

## 0.8.16: versions 6.0 through 6.4

Added independent observation code profiles, exact BATS fingerprints and retail release-event constructor audits for the five newly installed EXEs below. Their existing base profiles already identify these executables; this change adds the feature-level readers rather than treating base-profile recognition as feature support.

All five use 96-byte AI health components with a float maximum, the 47-byte compact scaled-game-time getter, the 516-byte weapon-raise calculation, packed animation-event disabled flags, and the single implicit `idMD6Anim* anim` release-event signature. The exact caller/helper locations, constants, reflected field operands, health ancestry and signature registration were checked independently in each EXE. Existing 1.0/1.1/5.1 and 6.66 observation profiles are preserved.

| Patch | PE timestamp | SizeOfImage | EXE SHA-256 | Offline audit |
| --- | --- | --- | --- | --- |
| DOOMEternal 6.0 | 0x60c98dd6 | 0x1cb22000 | `7abfc3fda951723636863d5722f266d31bb3236488c1e97df1f4301129d39d16` | passed |
| DOOMEternal 6.1 | 0x60dff532 | 0x1bc55000 | `1f972ea4cc13a3bfcba01577ca8fd4f3c35c6aec01cfc9468135dd8d19fe2031` | passed |
| DOOMEternal 6.2 | 0x60f26b2f | 0x1bb06000 | `3f86b9d3a0a3be5f2aedc8e22576c7787d6b93459d1a0bcaab5991fff34ea4f9` | passed |
| DOOMEternal 6.3 | 0x6116d6b5 | 0x1cd60000 | `be22cb6003c12b059ad02fd865208008f2f0489070098312b72f1ebe6b510930` | passed |
| DOOMEternal 6.4 | 0x61202c76 | 0x1d77e000 | `b5b27d2b023af52c12cbe0b5dbac47b9eea1072406932a0cff83396782993747` | passed |

Each new EXE passed the data-only production audit: real health/stagger metadata, full BATS code and dependencies, scaled/unscaled clock branches, lifetime override, release-event registration constructor and complete interaction/HUD routes. Synthetic objects use the EXE's real reflected layouts. Cases include Praetor, ordinary and Golden Gore Nests, arbitrary interaction names, script-started syncs, disabled events, pause, ownership release and corruption guards. Public clock, health and sync fixtures now cover all15 feature profiles.

All 23 native regression suites passed with the new profiles. The launcher and native layer were rebuilt, and the runtime package contains the tested layer.

This is offline verification of 6.0 through 6.4; no new game process was launched or controlled for this port. A live check is still required, starting with 6.0. The standard Meathook `XINPUT1_3.dll` is absent from all five newly installed directories, so metadata-based HUD features need that prerequisite before live acceptance. No installation files, saves, Wiki pages or user settings were modified. The local application preview version is 0.8.16; the release request remains unchanged.
