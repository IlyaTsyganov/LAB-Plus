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

Commands that require parameters show their own description and syntax when
called without them. For example, `labplus_trigger_set` prints
`<class> <0|1>` and explains that 0 hides the exact trigger class and 1 shows it;
it changes no settings until both parameters are supplied. Invalid parameters
also show command-specific help. Zero-argument actions such as
`labplus_recording_stop` continue to execute normally.

Supply only the class to read its current and factory-default visibility:

```text
labplus_trigger_set idTrigger
```

Queries use the game's standard two-line layout: the quoted name, current value
and `default` on the first line, with the description on the second line. HUD
queries also show factory defaults for each meter's position, size and color.
Saved preferences remain the current values; they do not replace factory defaults.

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
  state, unchanged factory defaults, alias synchronization, preservation of every unrelated setting and full
  restoration after the second change. This tests the bridge using a fixture;
  it does not execute the engine's `toggle` command.
- Launcher tests cover the actual handlers/persistence of all 21 main binary
  settings; HUD tests cover the remaining four settings.
- `CvarCatalogLiveProbe.exe <game-pid>` reads the actual game registry without
  executing commands, writing memory or changing settings. On 2026-11-27 all 45
  current names (44 binary names plus viewer distance) passed in game 1.1:
  current values, factory defaults, descriptions, types, ranges and binary
  completion values. In particular, Viewer retained current=1 and default=0.
- All 22 retail EXEs pass the production native cvar resolver and its changed-code
  guard with their images mapped as data only. Their reflected cvar fields,
  factory signatures, type flags and exact two-line query format also pass the
  read-only [compatibility audit](validation/cvar-query-compatibility.json).
  Run `tests/audit_cvar_query_abi.py` from `Native/CollisionViewer` with a local
  manifest and `--output <report.json>`; it requires `pefile`. The manifest contains
  the user's EXE paths and is not committed. No game code is executed by the audit.
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

Retail engines strip description and completion metadata from late-created cvars.
LAB Plus restores those two fields only in its own inline cvar storage, after
verifying that the data pointer belongs to that exact object. No foreign or shared
cvar storage is modified. The read-only catalogue probe validates current values,
factory defaults, descriptions and binary completion values in the live registry.
