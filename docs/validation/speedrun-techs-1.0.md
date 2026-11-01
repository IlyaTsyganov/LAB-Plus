# Speedrun technique observations

Observed read-only against DOOM Eternal 1.0, processes 13720 and 31616, on 2026-11-01.
The user performed each technique and confirmed the resulting game behavior.
The observer did not issue game inputs, console commands, or memory writes.

## BATS

The same equipped Ballista object and weapon declaration were retained across
all three stationary comparison snapshots. The user did not specify the first activation route; the confirmed repeat
used Heavy Cannon mod switching during a weapon swap.
The user cleared the effect using an ordinary weapon switch.

| Engine scalar | Active | After ordinary switch/reset | Active repeat |
|---|---:|---:|---:|
| `idAnimWebHands.bringUpAnimRate` | 5 | 1.6666666269302368 | 5 |
| `idAnimWebHands.bringDownAnimRate` | 1.6666666269302368 | 2.222222328186035 | 1.6666666269302368 |

The equipped weapon pointer was the same `idWeapon` with resource name
`weapon/player/gauss_rifle`. The observed active module was Destroyer Blade.
The reader checks the engine's current animation state. It does not infer BATS
from a history of keyboard actions, firing, or punch cancelling.

The initial reader was limited to these demonstrated Ballista states on 1.0.
The configured-rate reader below replaces that restriction. Punch cancelling
itself has no indicator.

The rates are not simply reciprocal weapon-declaration durations. Source
inspection found normal-rate calculations that include animation clip lengths,
weapon mod rate scales, dexterity upgrade speed, player speed clamping and
status-effect multipliers. The follow-up reproduces audited branches and leaves
unsupported configurations unavailable.

### Configured-rate follow-up

The follow-up reader removes the Ballista name/rate-pair gate. For an equipped
inventory weapon with `desiredBringupDurationMS.milliseconds == 0`, the audited
normal calculation starts at rate 1 and computes:

`normal = bringUpRateScale / max(max(dexterityWeaponChangeSpeed, FLT_EPSILON), weaponSwitchClampScale)`

This branch currently requires no active status effects and switch-rate mode 0.
Both fire-mode mod scales must agree, avoiding an ambiguous comparison when a
fire-mode change legitimately changes the configured scale. The current engine
raise rate must exceed the calculated normal value to report BATS. A matching
rate reports inactive; unsupported or unsettled values remain unavailable.

The ordinary upgraded Ballista observation gives `1 / max(0.6, 0.3) = 1.66667`.
A fresh-campaign Shotgun observation on process 31616 gives
`1 / max(1, 0.3) = 1`, matching its actual normal rate. This also prevents a
legitimate dexterity upgrade from becoming a false BATS indicator. Tests cover
multiple weapon names, absent Ballista, upgrade/no-upgrade configurations and
legitimate mod-rate overrides.

The user then confirmed ordinary Rocket Launcher, performed the Heavy Cannon
module-swap BATS route onto Rocket Launcher and verified the acceleration. The
same Rocket's actual raise rate changed from 1.6666666269 to 5 while its requested
duration stayed 0 ms, both mod scales stayed 1, dexterity stayed 0.6000000238 and
clamp stayed 0.3000000119. The computed normal value was 1.6666666004. The actual
production C++ reader reported active in 47 ms. This is a confirmed second weapon
using the generic calculation, without adding another weapon-name gate.

Nonzero requested bring-up durations additionally require the bring-up clip's
duration; that branch remains explicit until its bounded clip selection is
audited. The follow-up formula is audited on 1.0 and 1.1 (see the build audit below).

### Ordinary punch continuity (0.8.8)

The right-hand inventory item can temporarily be the ordinary punch item during
melee, so requiring an equipped gun for every sample interrupted the indicator.
When the reflected `isMeleeAnimPlaying` property is true, the reader can instead
use `idHands.meleeD5PrevWeapon`, provided it is still a valid inventory gun and
the actual hand item is either that gun or `weapon/player/fists_doom5melee`.
The previous-weapon pointer and melee flag are rechecked before returning.

This does not retain an old active label. Every accepted melee sample still
reads the current engine raise rate and compares it with the preceding gun's
current supported configuration. A reset to the normal rate immediately reports
inactive, including during melee. A queued ordinary weapon switch, missing
inventory reference, unrecognized getter or unsupported melee item remains
unavailable. Berserk and other status-effect cases keep the existing limitation.

Production-reader fixtures cover both handoff directions, repeated punches,
normal punches, a real reset during melee, a later punch after reset, stale
previous-weapon references, queued weapon changes and malformed metadata.
The optional live demonstration question received no answer; the read-only
120-second trace contained idle only and supplies no new live punch validation.

### DOOM Eternal 1.1 audit (0.8.9)

The actual 1.1 retail EXE (`timestamp=0x5ec7deca`, `imageSize=0x1d551000`)
was inspected and compared with the running image at PID 16488 on 2026-11-01.
Its mode 0 zero-duration formula is the same as 1.0. The version gate now selects
an audited code profile and validates the complete calculation and its helpers;
it does not accept a version name alone.

| Audited block | 1.0 RVA | 1.1 RVA | Bytes checked |
|---|---:|---:|---:|
| SetWeapon calculation and bring-up assignment | 0x161dbbb | 0x1659efb | 529 |
| Dexterity multiplier getter | 0x18beea0 | 0x1911620 | 44 |
| Base inventory declaration getter | 0x144a360 | 0x1c83140 | 5 |
| Status-effect switch multiplier | 0x1818fd0 | 0x1868230 | 143 |

The 1.1 calculation starts with 1, reads switch-rate mode 0, reads requested bring-up
milliseconds at declaration offset 0xb00, multiplies the active mode's override
at weapon offset 0x34a8 (stride 0x3d8), and divides by the clamped dexterity/status
multiplier. With no active status effects, the existing normal formula applies.
The constants 1, 0.001 and FLT_EPSILON are read from the actual RIP-relative targets
and checked in addition to the code fingerprints. Reflected desired-duration
and mod-scale displacements must still match the audited instruction operands.

The reflection reader correctly follows 1.1's changed layouts: player hands 50592,
hands webAnimator 4008, player dashboard 91048 and playerProps 185096. Those values
are supplied by metadata rather than substituted into the observer as 1.0 offsets.
The current ordinary 1.1 intro-level state had bring-up rate 1, no dexterity upgrade,
clamp 0.3 and mode 0. The actual production C++ reader reported BATS inactive in 47 ms
with no missing schemas. This is a normal-state live check; an activated 1.1 BATS
demonstration has not yet been performed.

Fast DBlade's 1.1 weapon-mod time types, 984-byte override stride and charge fields
remain compatible. The actual reflected `modChangeAnimPlaying` getter at
RVA 0x1da49d0 is `0f b6 41 05 c1 e8 07 c3`; `isMeleeAnimPlaying` at 0x1db8c70 is
`0f b6 41 02 c1 e8 04 83 e0 01 c3`. Both decode within the seven-byte flag object.
The production reader correctly reported Fast DBlade inactive when Ballista was
absent. Fixtures separately cover 1.1 prepared-hit activation, ordinary mastery
as a negative control, consumption and melee/reset behavior.

Audit artifacts: `metadata11.json`, `runtime11.json`, `flags11.json`,
`bats-calc11-disassembly.txt`, `bats-code-profiles.json` and
`current11-baseline.jsonl` and `production11-proof.txt` in `build/tech-research/`.
All four 1.1 code blocks
matched their live process bytes exactly. Tests embed the actual read-only
calculation/helper bytes from both retail versions and reject altered code,
helper instructions or constants. Nonzero desired durations and active
status-effect configurations keep their existing explicit limitations.

Evidence in `build/tech-research/`:

- `bats-active-animation.json`
- `bats-reset-animation.json`
- `bats-repeat-active-animation.json`
- `bats-active-hands.jsonl` and bounded 37,200-byte hands snapshots
- `rocket-ordinary-baseline.jsonl`, `rocket-bats-active.jsonl`
- `rocket-bats-demo.jsonl.gz`

## Fast Destroyer Blade

The user selected mastered Arbalest, performed the Heavy Cannon/mod-switch
sequence, hit an enemy with Arbalest and selected Destroyer Blade. The user then
confirmed the first Destroyer Blade charge and shot were fast.

| Engine field on Ballista's secondary mod | Ordinary Arbalest | Armed Destroyer Blade | After fast charge and shot |
|---|---:|---:|---:|
| `weaponModOverrides[1].chargeGameTime` | -1 | 1,999,920 | 1,999,920 |
| `weaponModOverrides[1].chargeGameTimeOnce` | -1 | 999,960 | -1 |

The unit is game time at 999,960 ticks per second. The armed state's actual
secondary weapon declaration was `weapon/player/gauss_rifle_destroyer`.
Therefore the next-charge override represented 1 second against the current
Destroyer Blade's normal 2 seconds, and disappeared after consumption.

The initial reader required the Destroyer Blade declaration and a positive next
charge duration shorter than its current normal duration. That delayed the
indicator until module selection and did not meet the desired activation time.

### Activation at the prepared Arbalest hit

Three further controlled observations on process 31616 separate the technique
from ordinary Arbalest mastery:

| Observation at successful Arbalest hit | `chargeGameTimeOnce` | `idHands.handsFlags.modChangeAnimPlaying` |
|---|---:|---|
| Ordinary mastery hit | 999,960 temporarily | false |
| Hit after interrupted mod-change preparation | 999,960, retained after recharge | true |
| Hit after a complete ordinary mod switch | 999,960 temporarily | false |

The normal one-second override disappears as the ordinary automatic recharge
finishes. Therefore `chargeGameTimeOnce > 0` alone is insufficient. The prepared
case still has the engine's mod-change animation flag set while its actual hands
state is `charge_shoot` / Arbalest; this supplies immediate preparation evidence.
The unrelated `newModIsUsable` bit also changes during ordinary mod switches and
is not used as evidence.

The observer decodes the reflected bool getter's leaf instructions to obtain
the packed bit; it never executes the getter or assumes an offset from a version
name. The observed 1.0 getter is `0f b6 41 05 c1 e8 07 c3`, identifying byte 5,
bit 7 inside the reflected seven-byte flags object.

The follow-up reader records this engine evidence as soon as the prepared hit
sets its one-shot override, without a wall-clock debounce or keyboard-sequence
inference. The record belongs to the map, world, player, inventory weapon and
managed reference token, and is tied to the secondary shot timestamp and the
override value. It remains active when another weapon is held and is cleared
when the override is consumed, a new shot replaces its provenance, or identity
changes. Merely selecting Destroyer Blade is no longer the activation trigger.

An already selected Destroyer Blade still permits direct recovery from a shorter
positive next-charge override. An Arbalest preparation that predates observation
is left unavailable when its original engine evidence was missed.

Additional evidence in `build/tech-research/`:

- `normal-arbalest-control-hit.jsonl.gz`
- `prepared-arbalest-hit.jsonl.gz`
- `consume-and-ordinary-modswitch-control.jsonl.gz`
- `hands-flags-runtime.json`
- `prepared-arbalest-after-hit.jsonl`
- `ordinary-complete-modswitch-after-hit.jsonl`

`weaponModOverrides.weaponModMastered` was false for the user's mastered
Arbalest; it is not a reliable ownership/mastery check and is not used as one.
The actual shorter one-shot duration is the evidence for the active effect.

Evidence:

- `ordinary-arbalest-baseline.jsonl`
- `fast-dblade-armed.jsonl`
- `fast-dblade-consumed.jsonl`
- `fast-dblade-demo-1.jsonl.gz` (16,228 weapon samples in 300 seconds)
- `fast-dblade-hands-demo-1.jsonl.gz` (17,786 hands samples in 300 seconds)

`SpeedrunTechTests` exercises both production readers using reflected-layout
fixtures, including activation/reset, consumption, unsupported builds/rates,
pending switches, temporary effects and incorrect time metadata.

The production C++ `FastBladeTracker` also replayed all 14,433 samples from the
three controlled follow-up traces with zero expected-state mismatches. Its first
active sample in the prepared trace was exactly the first hit-override sample;
neither ordinary control activated. Replay inputs and the standalone runner are
in `build/tech-research/tech-classifier-replay.tsv` and `tech-replay.cpp`.
