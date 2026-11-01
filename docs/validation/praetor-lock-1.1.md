# Praetor pickup: player release, game 1.1

This audit only reads the executable and process memory. No input, console command, game function invocation, memory write or checkpoint reload was performed by the observer. The user performed the pickup and then walked normally.

## Why the whole clip is wrong

The confirmed capture is `build/praetor-lock-1.1-research/praetor-demo-1.jsonl.gz`, PID 792, game 1.1 timestamp `0x5EC7DECA`, image size `0x1D551000`. The pickup is `pickups_progress_praetor_token_2_e3m2`; its sync is `interact/preator_suit_token/preator_suit_token_sync_138`.

At game time 22,000,876 the player entered the sync. The master animation and NPC participant both have 200 frames at 30 fps, giving a final-frame time of 6.633333 seconds. The player regained the saved animation abilities after approximately 3.1476 seconds while the sync and NPC animation were still working. Thus the current master/NPC clip end cannot represent return of player control.

| Capture point | Scaled game ticks | Evidence |
| --- | ---: | --- |
| Pickup starts | 22,000,876 | Player abilities become 64; saved abilities are 4095; player participant points to the player |
| Last complete player participation sample | 25,124,059 | Player participant still present; abilities 64 |
| Release is being processed | 25,136,123 | Player participant becomes null; abilities still 64 in this non-atomic observation |
| First complete restored sample | 25,148,326 | Player abilities 4095; player participant null; NPC sync still working |

The pickup does not set `inhibitFlags & 7`, `disableMovementDuringCustomAnim` or `inInteractionView` in this trace. Those all remain zero. Treating one of these fields alone as the pickup's active control lock would therefore be incorrect. The managed animation-ability deadline list also remains empty and cannot provide this pickup's deadline.

## The actual release event

The relevant animation is the **player model** in `idSyncEntity.animatorSyncEntity.modelAnimStates`, not the master animation stack and not the player's ordinary third-person idle/walk stack. The player model's active animation is:

`md6/player/human/base/motion/mechanics/interact/preator_suit_token/preator_suit_token_player.md6anim`

Its leaf has the same 22,000,876 start time, playback rate 1, all game-rate scales 1, `startTimeFraction = -1`, `syncDataIndex = -1` and 999960 ticks per second. After release its processed-event frame remains 94 while the master and NPC timelines continue. The model's MD6 declaration, resolved from the sync animator's corresponding model definition, contains a future event at **frame 94** named **`ae_releasePlayerFromSync`**. The definition event is available before playback; it is not inferred from elapsed observation time. Evidence: `player-model-events.json` and `current-component-events.json` in the same research directory.

The current executable's global event registry confirms the names behind numeric event references:

| Event number in this process | Engine name | Role |
| ---: | --- | --- |
| 179 | `ae_sound` | NPC sound events |
| 198 | `ae_disableClip` | NPC frame-94 collision event; not a release event |
| 223 | `ae_hideMesh` | NPC frame-94 mesh event; not a release event |
| 248 | `ae_hudHide` | Master frame 0 |
| 249 | `ae_hudShow` | Master frame 90 |
| 565 | `ae_syncEnd` | Master frame 199; not the earlier player release |
| 691 | `ae_releasePlayerFromSync` | Player model frame 94 |

The numbers above are observed registry indexes, not portable constants. The event reader must identify the name through the actual guarded registry and validate the declaration's event fields. Matching an observed NPC frame or hardcoding 198 as release would produce a false result.

The registry lookup is verified in game 1.1 function RVA `0x44F870`. It bounds the supplied event number at 4095 and indexes the pointer array at RVA `0x3D85C70`. The array address is decoded from the original RIP-relative LEA, rather than accepted without its getter-byte guard. Reflected `idEventDef` is 96 bytes: `name` at 0, `eventnum` at 52 and `type` at 60. A resolved MD6 release event must name `ae_releasePlayerFromSync`, have matching event number and animation-event type 2. Source: `eventdef-index-getter.txt`, `event-registry.json`, `eventdef-metadata.json`.

The same 31-byte getter shape was independently found exactly once in each previously supported executable. Its only variable bytes are the RIP-relative array displacement at offset 18; the target is `getter + 22 + displacement`. Executable identity and the original instruction bytes still require verification before reading the array.

| Executable | Getter RVA | Registry array RVA |
| --- | --- | --- |
| 1.0 Release | 0x44B2F0 | 0x3D14370 |
| 1.1 | 0x44F870 | 0x3D85C70 |
| 5.1 | 0x474360 | 0x408B4B0 |

Source: `event-getter-profiles.json`. This compatibility check audits event lookup code on 1.0/5.1; the positive pickup demonstration and timing evidence below are from 1.1.

## Release callback semantics

`idAlignedEntity::AnimEvent_ReleasePlayerFromSync` is identified by its original diagnostic name and code at RVA `0xFA22E0`. It resolves the sync and the aligned entity's player owner and invokes the sync's player-release path at `0x117A850`. That path calls the participant wrapper `0xFBCCA0`, which dispatches virtual slot `0x13C0`.

For the actual 1.1 player vtable at RVA `0x2984578`, slot `0x13C0` points to `0x16C66A0`. This implementation restores `animAbilitiesSaveSync` into `animAbilities` at `0x16C6FA5`/`0x16C6FB0` and clears the player's inhibit flags at `0x16C7014`. The participant is then released from the sync. The trace's player removal and restoration of saved abilities agree with this implementation, including the interval where a read crosses the update and observes only one of the two changes.

The separate `idPlayer::ReleaseFromSync` implementation at `0x16D90D0` is also identified by its diagnostic string. It can call the same sync release path through current/saved sync references. Its event wrapper is `0x16B70C0`. These establish that control release is an explicit engine transition; observing a continuing NPC clip does not mean the player is still held.

Source disassembly is in `release-player-from-sync.txt`, `release-sync-player-call.txt`, `release-participant.txt`, `player-release-flags.txt` and `player-release-entry.txt` under the research directory.

## Time conversion and limits

Game 1.1's floating event-frame getter at RVA `0x5F5190` uses scaled game time and the leaf's actual playback rates. For the ordinary negative-`startTimeFraction` path:

1. `effectiveRate = (((scale[2] * scale[0]) - 1) * scale[1] + 1) * rateScale`.
2. The elapsed tick count is converted to float, multiplied by that rate and truncated back to integer ticks.
3. The frame is the float conversion of `fps * scaledElapsedTicks / ticksPerSecond`, clamped for a non-looping leaf.

The effective-rate helper is at `0x5D29B0`. Source: `leaf-event-frame-formula.txt` and `leaf-effective-rate.txt`. An event frame uses its authored frame number directly; the clip-end `numFrames - 1` calculation must not replace it.

At the captured unit rate, frame 94 is reached after `94 / 30 = 3.133333...` game seconds, with nominal event time 25,134,084 ticks. The engine processed the release in the update observed at 25,136,123 ticks, followed by a complete restored-player observation at 25,148,326. The countdown describes the authored engine release event; update scheduling and a non-atomic read can move the observed transition to the next frame. No measured-duration correction is added.

Only a valid current-player participant and its matching active model/leaf, current declaration event, compatible time domain and stable references can establish the countdown. Ambiguous model bindings, unknown release events, looping or unsupported time mappings, stopped/invalid rates, or changed guarded code must remain unavailable. Once the player participant has been released, a still-working master/NPC timeline must not keep the player-lock timer active. This audit establishes the demonstrated 1.1 Praetor path; other interaction types require their own matching release event or authoritative engine deadline.
