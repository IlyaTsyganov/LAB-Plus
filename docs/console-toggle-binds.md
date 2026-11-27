# Binary console settings

Use the standard engine syntax, without explicit values:

```text
bind x "toggle labplus_viewer"
```

The preferred names match `labplus_commands_list`. Each short name below is a
synchronized native cvar alias and also works with `toggle`. These are 25 settings
with 44 distinct names, on all 22 audited retail profiles.

`labplus_commands_list` describes each setting and its range. Binary cvar value
completion offers only `0` and `1`; `toggle` is a separate command before the
cvar name, rather than a value after it.

| Setting | Preferred cvar | Short alias |
| --- | --- | --- |
| Viewer master switch | `labplus_viewer_show` | `labplus_viewer` |
| Triggers | `labplus_triggers_show` | `labplus_triggers` |
| World collision | `labplus_collision_show` | `labplus_collision` |
| Shot collision | `labplus_shots_show` | `labplus_shots` |
| Invisible walls | `labplus_walls_show` | `labplus_walls` |
| Volume inspector | `labplus_inspector_show` | `labplus_inspector` |
| Disabled volumes | `labplus_disabled_volumes_show` | `labplus_disabled` |
| Volume history | `labplus_volume_history_show` | `labplus_history` |
| Research path | `labplus_path_show` | `labplus_path` |
| Player collider | `labplus_player_show` | `labplus_player` |
| Support plane HUD | `labplus_surface_show` | `labplus_surface` |
| Attempt motion paths | `labplus_attempts_show` | `labplus_attempts` |
| Active encounters HUD | `labplus_encounters_show` | — |
| Filled/outline rendering | `labplus_viewer_fill` | `labplus_fill` |
| Enemy health HUD | `labplus_enemies_show` | `labplus_enemies` |
| Enemy health bars | `labplus_enemy_hp_bars_show` | `labplus_enemy_hp_bars` |
| Player coordinates | `labplus_position_show` | `labplus_position` |
| Player angles | `labplus_angles_show` | `labplus_angles` |
| Game timers | `labplus_timers_show` | `labplus_timers` |
| Speedrun technique HUD | `labplus_techs_show` | `labplus_techs` |
| Research recording | `labplus_record` | — |
| Speedometers master switch | `labplus_speedometer_show` | — |
| Landing speed | `labplus_speedometer_landing_show` | — |
| Vertical speed | `labplus_speedometer_vertical_show` | — |
| Signed vertical speed | `labplus_speedometer_sign_vertical` | — |

`labplus_encounters` is an encounter-list command. Use
`toggle labplus_encounters_show` for its visibility setting. Numeric settings such
as viewer distance, HUD scale, tint opacity, meter position/size/color are not
binary settings. Per-trigger-class controls take additional arguments and are
commands, rather than independent cvars.

## Verification

- `LabCvarTests.exe` changes each of the 44 binary native names in both directions,
  verifies one numeric companion request per change, acknowledgement and saved
  state, alias synchronization, preservation of every unrelated setting and full
  restoration after the second change. This tests the bridge using a fixture;
  it does not execute the engine's `toggle` command.
- Launcher tests cover the actual handlers/persistence of all 21 main binary
  settings; HUD tests cover the remaining four settings.
- `CvarCatalogLiveProbe.exe <game-pid>` reads the actual game registry without
  executing commands, writing memory or changing settings. On 2026-11-27 all 38
  names were found in the running 1.1 game with BOOL/NOCHEAT flags, 0..1 bounds
  and binary values. The probe validates the production ABI before reading the
  registry and fails if any name is missing or has the wrong type. That live
  snapshot predates the six additional `_show` names and the three speedometer
  renames; the updated 44-name catalogue is covered by the round-trip tests.
- The physical `bind x "toggle labplus_viewer"` round trip was confirmed in
  gameplay on 1.1. Individual physical binds for the other names and gameplay on
  the other 21 patches have not been exercised in this check.

## Recording actions

`labplus_recording_start` starts/resumes recording and `labplus_recording_stop`
stops it. `labplus_record` is the binary cvar: `1` starts, `0` stops, and
`bind x "toggle labplus_record"` switches between them.

After updating an already running game, restart it through the updated LAB Plus
build to load the renamed cvars and speedometer commands. Old registered command
name buffers are not renamed in place when reconnecting.
