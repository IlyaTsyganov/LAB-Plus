# Sync interactions on game 1.1

The former capture observed `idPlayer.syncMaster`, `idSyncAttack.currentSyncEntity` and `idPlayerMechanicSync.syncAnimEntities`. It returned only one active timeline, and the companion interaction row reused that countdown when `interactAnimIsPlaying` was true. This could omit an interactable-owned sync or label a melee countdown as a button.

## Additional source

The actual 1.1 executable reflects this separate ownership path:

`idPlayer.lastUsedInteractable -> idInteractable.interaction -> idInteractionComponent.activeSyncEntity`

The inline component also exposes `ownerEntity` and `playerActivator`. Capture follows its active sync only when the owner is the referenced interactable and the activator is the current player. The sync must be alive, working and owned by that same player. A removed last-used object, a finished sync or a different player's interaction cannot remain active merely because the historical reference survives.

Verified reflected 1.1 layout values:

| Type | Field | Offset |
| --- | --- | ---: |
| idPlayer | lastUsedInteractable | 0x5ae0 |
| idInteractable | interaction | 0x1b38 |
| idInteractionComponent | activeSyncEntity | 0x988 |
| idInteractionComponent | playerActivator | 0x9a8 |
| idInteractionComponent | ownerEntity | 0x950 |
| idSyncEntity | syncPlayer | 0x1b00 |
| idSyncEntity | isCurrentlyWorking | 0x27e8 |
| idSyncEntity | currentSyncEntityAnim | 0x27f0 |
| idSyncEntity | isInteractableSync | 0x2c44 |

These are evidence for the reflected runtime reader, not hardcoded offsets in the production capture. `savedSyncEntity` and `mostRecentSyncEntity` are also considered, subject to the same active/player checks. `idPlayerMechanicInteract` is absent from 1.1 metadata; assuming its later-version layout would not provide this route.

## Timeline and labels

`captureSyncAnimationTimers` returns separate rows for distinct active sync entities and deduplicates references to the same entity. Melee is `sync_animation`, interaction is `interaction`, and other player-owned scripted syncs use `sync_other_N`. A glory-kill countdown is never copied into a separate interaction row.

The value is explicitly the **current animation clip**, not a promised whole-interaction deadline. It reads the current MD6 animation, bounded active leaf tree, frame intervals, playback rates and engine clock. The maximum is that same clip's actual scaled duration. Future graph transitions, loops and uncertain overlapping timelines are not converted into guessed totals. Mantle has its own verified ledge-grab deadline in the shared timer reader.

The final capture rechecks managed references and tokens, list identity/count, owner and working state, selected animation/tree, pause/local-time state, animation resource data, and accepted leaf start/rate/scale fields. Unsupported local clocks and looped clips stay unavailable. No game functions are called by this observer.

## Validation

- `SyncInteractionTests` compiled with MSVC C++17 `/W4` without warnings and passed. Cases include an interaction with empty master/melee lists, source deduplication, two simultaneous independent syncs, no melee-to-interaction copying, real remaining/maximum values, paused time, local clocks, looping, different players, removed objects and missing optional metadata.
- The direct production reader accepted all 60 runtime schemas from game 1.1, read a valid scaled clock and returned both rows inactive with no missing metadata. Evidence: `build/sync-1.1-research/production-idle-proof.txt`.
- The bounded read-only watch recorded 8,614 samples with zero read errors. The last-used interaction component's owner and activator matched the actual object and player; its active sync was null, and no active interaction or mantle was recorded. The demonstration question received no answer. This is an idle/ownership proof, **not a positive live button or pickup demonstration**.

Research schemas are in `build/sync-1.1-research/initial.json`, `interactions.json` and `runtime-layouts-1.1.tsv`. The optional watch exited after 180 seconds; no helper was left running.
