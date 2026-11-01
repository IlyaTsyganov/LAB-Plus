# Compatibility

Support is checked against the **actual executable**, not its folder name. A recognized collision profile does not establish every timer, console command or meathook feature.

## Collision profiles

The collision reader recognizes 22 executable profiles:

| Family | Versions |
| --- | --- |
| Release | 1.0, 1.1 |
| Updates 2–3 | 2.0, 2.1, 3.0, 3.1 |
| Updates 4–5 | 4.0, 4.1, 5.0, 5.1 |
| Update 6 | 6.0, 6.1, 6.2, 6.3, 6.4 |
| 6.66 | 6.66, Rev 1, Rev 1.1, Rev 2, Rev 2.2, Rev 3, Rev 3.2 |

Unknown or changed layouts can withhold rendering. Loaded map geometry is discovered at runtime; the profile count is not a claim of live acceptance on every map, GPU and driver.

## Feature matrix

| Feature | Supported scope and requirements |
| --- | --- |
| World collision, triggers, invisible walls | A recognized collision profile, valid camera/physics data and the loaded current layer |
| LAB speedometers, MR, changed cvars, player-cheat states | Check the current feature status on LAB; independent of Research meathook |
| Research metadata/HUD and console bridge | Compatible, already-loaded meathook; not bundled and not implied by the 22 collision profiles |
| Equipment, powerup and animation timers | **1.0, 1.1 and 5.1**; unsupported game data is shown as Unavailable |
| Enemy HP/stagger panels | Valid live enemy/position data; GK countdown sources verified for **1.0, 1.1 and 5.1** |
| BATS | **1.0 and 1.1**, with supported weapon raise-time settings and no active status effects |
| Fast DBlade | Supported Ballista preparation/charge states; see the technique limits below |
| Player lock | **1.1 only**, the supported Praetor Suit Point pickup; counts down until player control returns |
| Other interaction/sync timers | Verified **current clip**, not the whole scripted sequence or a general control-unlock deadline |
| Persistent Research cvars | **6.66 Rev 3 only**; UI and short actions have separate availability |
| Lighting Fix | **NVIDIA, verified 1.0/1.1/2.0**, launched through LAB Plus |
| Live checkpoint loading | **6.66 Rev 3 / TAG1 / slot 3**, same account, Steam Cloud off and all checks passing |

### Technique limits

BATS can show Unavailable with custom weapon raise durations, active status effects or unsupported weapon/mod settings.

Fast DBlade needs an observed prepared hit. Ordinary mastery recharge does not activate it. A preparation made before LAB Plus began observing can remain Unavailable.

### Timer limits

Equipment is Ready only when it is acquired and usable. Some timers cannot show a reliable countdown or maximum duration and remain Unavailable or omit the maximum.

Player lock is available for the supported Praetor pickup on 1.1. Other interactions show the current animation clip's remaining time, which can differ from when control returns.

### Native cvars versus action commands

A persistent cvar may be unavailable while the UI and short actions still work. Use an action such as `labplus_distance 60` or `labplus_timers 1` when game console integration is available. Do not create unsupported variables with an invented `set` command. LAB speedometer commands work separately.

## Lighting and launch policy

Lighting Fix stays off on AMD, Intel, unknown hardware and other game versions. Disabling Collision Viewer does not broaden that policy.

Every launch path still requires an effective outbound firewall block for the selected EXE. See [Installation](Installation-and-first-launch).

Read an Unavailable item's status/detail and consult [Troubleshooting](Troubleshooting) when a feature does not appear.
