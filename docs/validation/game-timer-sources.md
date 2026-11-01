# Native timer observations

The timer reader only copies game data. It does not call engine getters, issue commands, change player state, or start timers from input events.

## Time basis

Equipment deadlines use scaled game-time domain 1, with the reflected `gameTimeUnique_t` unit of 999960 ticks/second. The reader verifies the original flame-belch recharge caller and clock getter before following their map/time-manager pointers. It also requires the clock's map to equal the current level.

| Executable | Flame check caller RVA | Clock getter RVA | Map pointer RVA | Inline time manager | Scaling cvar pointer RVA |
| --- | --- | --- | --- | --- | --- |
| 1.0 Release | 0x18DB163 | 0xB514E0 | 0x6136A10 | 0x127EB8 | 0x61355E0 |
| 1.1 | 0x192C8D3 | 0xB5E450 | 0x6128718 | 0x127F68 | 0x61272B0 |
| 5.1 | 0x1AE67A3 | 0xC21100 | 0x658A5F0 | 0x1257E0 | 0x6588FB0 |

The getter selects `timeManager.data + 24 + 72` when scaling is enabled, and `data + 24` otherwise. These are audited source locations, not an offset-only compatibility promise. Modified getter bytes or other executable profiles leave deadline-based timers unavailable.

## Equipment and powerups

`idPlayer.equipmentInfo.equipmentItems` contains **three** entries on 1.0/1.1 and **four** on 5.1. Its reflected element size is 40 bytes. Slots 0, 1 and 2 are frag grenade, ice bomb and flame belch. The reader uses `equipmentRechargeTime`, `equipmentReuseTime` and available charge count; flame belch also respects `flameBelchNextUseTime`. Zero charges without an active recharge timer remain unavailable rather than claiming readiness.

Initialized recharge counters do not establish ownership. Equipment declarations are bound by `idPlayer.playerProps.validEquipmentList`, then matched by pointer against the player's current inventory. A matching item must have a positive count, point back to the current player and allow use on this map. Flame belch may have left/right declarations; either usable owned declaration is sufficient. Confirmed absent equipment is `not_acquired`; unreadable ownership remains unavailable.

The fresh 1.0 `e1m1_intro` save was checked read-only in `build/ownership-research/production-proof.txt`: its equipment counters were initialized, but none of the frag/ice/flame declarations were in the 15-item inventory. All three rows correctly reported **Not acquired**. The declaration mapping and inventory evidence are in `build/ownership-research/equipment-current-1.0.json`.

Powerups come from `idActor.statusEffectComponent.activeStatusEffects`. Only definitions with `idDeclStatusEffect.isPowerUp` are included. `idStatusEffect.timeLeft` is an existing remaining duration; paused values remain paused. Negative/unbounded values do not become invented countdowns.

The `inActive` byte has been checked against both game executables. The active predicate returns true when this byte is zero: 1.0 vtable RVA 0x2947408, slot 0x68, function 0x1819190; 5.1 vtable RVA 0x2BE4418, slot 0x68, function 0x1A0F760. The status-effect component calls this predicate before applying an effect.

## Sticky bombs: two mechanisms

The exact selected mod resource is `weapon/player/shotgun_secondary_pop_rockets`. Heat parameters come from the selected declaration and current fire-mode upgrade overrides, including increment, maximum and cooling rate. No fixed three/five-charge capacity or reload duration is assumed.

The base weapon must also be an owned, usable inventory item. A cached selected-mod declaration alone does not prove acquisition: the reader requires an active upgrade with positive upgrade/cheat count and a `WMT_WEAPON_DECL_WEAPON` modifier granting that exact declaration. The modifier enum value 40 is audited on 1.0, 1.1 and 5.1. Its fire-mode application is independent of ownership. Missing grant evidence stays unavailable, including unlock cheats that bypass this ledger; it never becomes a false "not acquired" claim.

When some charges remain, the next charge countdown reaches the next engine heat threshold, including the remaining cooling delay. When all charges are used, `overheatedDelay` is a separate remaining-duration counter and all charges return together. The two rows expose these phases independently.

The per-charge display is an **ordered recovery queue**, not independent grenade clocks. `sticky_charge_1` through `sticky_charge_N` in group `sticky_charges` describe only missing charges, numbered by recovery order. During partial regeneration, entry `i` completes at `nextCharge + i * heatIncrement / heatDecrementRate`. Firing again or changing the effective recovery rate updates the shared queue. During an empty-magazine reload, all missing charges show the same `overheatedDelay` and explicitly share one completion event. No timer is created for an already-ready charge.

Read-only 1.0 user demonstrations were captured in `build/telemetry-research/sticky-demo-1.jsonl.gz` and `sticky-empty-demo-1.jsonl.gz`:

- Three upgraded shots raised heat 0.2 → 0.4 → 0.6, followed by individual regeneration. The normal effective cooling rate was 0.08/second; a current-mode override may change it.
- Exhausting all five charges raised heat to 1 and set `overheatedDelay = 1999920` ticks. At the end of this two-second counter, heat jumped from approximately 0.8673 to 0 and all five charges returned together.

## Animations

Mantles use the reflected active ledge-grab flag and `grabEndTime`.

Sync observations read all three reflected player sources: `syncMaster`, `syncMelee.currentSyncEntity`, and `playerMechanicSync.syncAnimEntities`. The legacy `syncMaster` field alone is insufficient on 1.0. The reader checks current working state, player ownership, stable managed-reference tokens and list identity; unavailable source metadata never means no active sync.

The selected `idSyncEntity.currentSyncEntityAnim` is matched to an active, clamped `idMD6LeafPlay` in that entity's animation stack. The displayed countdown is for the **current clip**, not future nodes in a multi-clip sequence. The end-time formula follows the game's 5.1 functions 0x627900 and 0x605E60, using frame count minus one, frame rate, start time and actual rate scales. Looping clips, local-time stacks, time-overridden branches and ambiguous overlapping timelines remain unavailable. An active interaction may use the same verified current-clip countdown. Focused fixtures validate these paths; the read-only live sync segment captured no user-confirmed positive action.

Focused native tests cover clock-domain selection, altered-getter rejection, equipment recharge with charges still available, direct powerup duration/pausing, malformed lists, map transitions, mantle deadlines, sticky partial/full regeneration, clamped sync clips and unsupported animation clocks.

## Maximum durations

`GameTimerRow.maximum` is an optional number of seconds (`-1` means unknown). It never substitutes the current remaining value or a sampled high-water mark for a missing total. A configured maximum shorter than the observed remaining time is withheld. Known values are sourced as follows:

- Equipment: the engine's stored `equipmentRechargeDuration` for that item. Before the first scheduled recharge this can still be zero; no fixed 20/30-second fallback is inserted.
- Sticky individual recovery: configured cooling delay plus each ordered recovery interval at the current effective rate. Queue maxima are cumulative because later missing charges recover sequentially.
- Sticky empty-magazine reload: the configured overheat delay and its upgrade override for `heatInfo.overheatFireMode`. The active primary-fire mode after releasing the mod does not select the secondary mod's reload duration. If both modes can create heat but have inconsistent durations, the maximum stays unknown.
- Powerups: activation's lifetime precedence: positive `statusEffect_lifetimeSec`, then the per-instance lifetime override, then the status-effect declaration's `lifeTimeMS`. These are current lifetime settings; unsupported/changed settings that cannot bound the remaining duration stay unknown. The global read is guarded by audited original instruction bytes at 1.0 RVA `0x1817FD1`, 1.1 RVA `0x18671F1` and 5.1 RVA `0x1A0E261`.
- Mantle: the difference between actual `grabStartTime` and `grabEndTime`.
- Sync/interaction clip: the full clamped clip duration at the same verified playback rate used for its remaining countdown. It does not include future nodes of a multi-clip sequence.

The millisecond declaration values are read directly rather than requiring the engine's lazily initialized game-time conversion cache. Tests cover the sequential 3.25/5.75/8.25-second queue, the shared two-second reload of all five charges, unknown maxima, per-mode reload overrides, and lifetime override precedence.

Read-only production capture on the full 1.0 Fortress save is saved in `build/timer-maximum-research/production-proof.txt`: the selected upgraded Sticky mod exposes a 3.25-second maximum for the next individual charge and a two-second full reload. With all five charges ready it emits no missing-charge rows. Equipment maximum fields remain unknown until the engine stores a recharge duration; they are not replaced with fixed guesses.

## 1.1 compatibility audit

The 1.1 executable is identified by timestamp `0x5EC7DECA` and image size `0x1D551000`. Its clock locations above were decoded from that executable's flame-belch comparison, using the actual reflected `flameBelchNextUseTime` displacement `0x6628`. The getter's original 54 bytes match the audited scaled/unscaled domain selection. The powerup lifetime instruction resolves to cvar pointer RVA `0x61B87C0` and reads the float at offset 12. No earlier version's addresses are reused for this profile.

Focused profile fixtures verify all three audited executables, both scaling branches, reflected-field mismatch, foreign-map pointers, changed caller/getter bytes, out-of-image globals and executable identity mismatch. The actual 1.1 metadata also confirms the three-entry equipment array and `WMT_WEAPON_DECL_WEAPON = 40`.

Read-only production capture against PID 16488 is saved in `build/timer-1.1-research/production-intro-proof.txt`. It validates the current map's scaled clock and reads `statusEffect_lifetimeSec = 0`. On that `e1m1_intro` snapshot equipment is correctly not acquired, no powerup or mantle is active, and the acquired Sticky mod is ready with three charges, a 4.916667-second next-charge maximum and a 2.5-second full reload maximum. All requested timer schemas resolve from current runtime metadata. This is a positive clock/source/ready-state check; it does not establish live equipment cooldown, active powerup, mantle or sync countdown behavior on 1.1 without corresponding user actions.
