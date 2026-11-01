# Stagger deadline validation on 1.1

The 1.1 executable previously lacked an audited `idAIVolatile` getter pair in `resolveEnemyStaggerPointer`. Reflection still exposed the AI and pain-state fields, but the reader correctly withheld the countdown rather than following the 1.0 or 5.1 code addresses.

The 1.1 profile now selects these verified vanilla getters:

| Source | RVA | Operation |
| --- | --- | --- |
| Pain state | `0xbc3f00` | Read `idAI2.aiVolatile` at reflected offset `0x58a0`, add `0xe440` |
| Deadline | `0xbc3fb0` | Read the same pointer and its `0xe500` deadline |
| Reaction | `0xbc3340` | Read `currentPainReaction` from that pain state |
| Recovery guard | `0xe915c0` | Accept `PAIN_STAGGER` (16) or `PAIN_STAGGER_VULNERABLE` (17) |

The reflected `aiPainState_t` is 3544 bytes. `currentPainReaction` is at offset 0, `finishStaggerTime` at `0xc0`, and `staggerLength` at `0xc8`. Both time fields use `gameTimeUnique_t`, 999960 ticks per second. The actual stagger-state entry selects scaled game-time domain 1 when comparing its finish deadline.

The production resolver validates the complete small getter bodies, their AI-pointer displacement against reflection, and the relationship between the pain-state base and reflected deadline offset. A mismatched executable image size or changed getter bytes disables only the countdown.

## Read-only live baseline

Process 16488, base `0x7ff7afe30000`, was inspected on Hell on Earth. Both getter bodies matched the 1.1 executable on disk. Three live zombies had HP `200/200` and valid world bounds. All three had reaction 0 and deadline/duration `-1`, correctly identifying an inactive stagger window rather than a zero-second active timer.

The local evidence is in `build/stagger-1.1-research/memory-proof-16488.json`; the associated runtime class layouts and address file are in that directory. No input, console command or game-memory write was performed by this baseline probe.

`EnemyTelemetryTests` now runs its health, position, stagger and changed-getter cases against the 1.0, 1.1 and 5.1 profile selectors. It includes real recorded reaction-16 deadline/clock regression pairs, reaction 17, paused and unavailable clocks, expiration, and negative flinch/falter/stun cases.
