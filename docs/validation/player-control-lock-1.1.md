# Player control lock on game 1.1

The previous Interaction countdown measured the Sync entity's current animation clip. A Praetor Suit Point demonstrates why that is the wrong boundary for player control: its master, player and object clips all contain 200 frames at 30 fps, but the authored player-release event occurs on frame 94. The NPC's remaining animation continues after control has already returned.

## Positive observation

The user performed a new Praetor pickup and held movement until walking resumed. The bounded read-only watch recorded 15,980 samples. The game was manually reloaded during the watch, so loading samples can be unavailable; no game state was changed by the observer.

| Observed state | Scaled game ticks |
| --- | ---: |
| Player-model leaf start | 22,000,876 |
| Authored release deadline: start + 94/30 * 999960 | 25,134,084 |
| Last sampled player abilities still 64; player participant removed | 25,136,123 |
| First sampled abilities restored to 4095 and saved sync cleared | 25,148,326 |

The expected event deadline precedes observed dispatch by 2,039 ticks (2.039 ms); this is normal next-update dispatch, not a fitted correction. The hidden-weapon animation starts returning at 25,140,171. The maximum countdown is 3.133333 seconds. The old clip endpoint would be 6.633333 seconds.

`inhibitFlags` stayed zero throughout this particular interaction and the managed-abilities deadline list stayed empty. They cannot be used as a fabricated Praetor deadline. The authoritative binding is `savedSyncEntity` for this path; it clears when the real player-release operation restores control. `mostRecentSyncEntity`, the component's active sync, and the NPC animation survive longer and must not keep the lock countdown visible.

Evidence: `build/praetor-lock-1.1-research/praetor-demo-1.jsonl.gz`, `praetor-transitions.json`, `boundary-samples.json`, `current-model-states.json` and `player-model-events.json`.

## Future deadline source

Capture follows the reflected parallel model collections:

`idSyncEntity.animatorSyncEntity -> idAnimator_AnimWeb.modelAnimStates`

`idAnimator_AnimWeb.decl -> idDeclAnimWeb.models -> idDeclAnimWeb::idModelAnimInfo.model -> idDeclMD6.animEvents`

Each active model tree is paired with its actual MD6 declaration. In the observed case, model 1 uses `md6def/player/human/base/tp_body.md6`. Its active player animation has an enabled, unconditional `ae_releasePlayerFromSync` event on frame 94. This declaration is available before playback reaches the event; the countdown does not wait to discover a triggered event.

The release calculation does not hardcode frame 94 or a 3.13-second duration. A separate compatibility selector limits the new Player lock display to the positively audited Praetor animation asset on game 1.1; other interactions retain their explicitly named current-clip countdown. It discovers the release event number from the engine's indexed event-definition registry, validating the original lookup getter bytes, the index, the exact event name, its animation-event type and zero arguments. The event is number 691 on this observed 1.1 process. NPC events 198 (`ae_disableClip`) and 223 (`ae_hideMesh`) also happen at frame 94, but are not the player-release event and are excluded.

The callback audit connects `ae_releasePlayerFromSync` to the actual Sync/player release operation. It restores saved animation abilities and clears input inhibits. The player-model event supplies the future deadline; the player's `savedSyncEntity`/`syncMaster` references supply the current lock state. A cleared binding makes the row inactive immediately, even if the old NPC Sync entity is still working.

## Clock and bounds

The audited normal MD6 leaf frame calculation uses the scaled domain-1 game clock and effective rate:

`effective = (((scale2 * scale0) - 1) * scale1 + 1) * rateScale`

The event deadline is computed from the event frame and actual frame rate, with upward tick rounding. Only the validated negative `startTimeFraction` branch and 999960-tick domain are accepted. Fractional-start branches, different tick domains, local animation clocks, unsupported branch time overrides, loops, conditional/disabled release events, missing metadata and ambiguous model deadlines remain unavailable. Pause state is retained.

All model-tree, declaration-info and event iterations share a 20,000-step bound. Managed identities, model/declaration lists, leaf start/rates/scales, resource frame counts, event state and pause state are rechecked before publication.

## Scope and checks

The new **Player lock** row is enabled for the audited Praetor asset on game 1.1, where the full model/release path has positive validation. This asset selector is an audit-scope guard, not a fixed-duration table: the event identity, frame, start time and rates still come from the current engine resources. Other 1.1 interactions and the 1.0/5.1 builds retain their explicitly labeled current-clip interaction countdown until their player-control paths are validated. The event-registry getter decoder itself is audited for all three timer builds.

`SyncInteractionTests` covers the recorded start/deadline/pre-release clocks, a release in the second model instead of the NPC clip, suppression after the saved binding clears while the NPC remains working, registry opcode/index/type checks, unrelated NPC events, unsupported tick domains and fractional starts, local clocks and scan-budget exhaustion. Existing other-profile current-clip behavior is retained.

An optional mixed live/read-adapter replay was prepared but not used as validation: by the time it ran, the actual interaction component had cleared its active Sync pointer. The probe stopped at that safeguard instead of reading a historical freed object. Recorded numeric/model inputs are replayed through the production helper in the deterministic regression fixture. No additional pickup was requested merely to repeat an already recorded action.
