# Console commands and cvars

Keep LAB Plus open for the selected game and load the matching current layer at full game startup. All LAB Plus controls use **labplus_**; old lab_ prefixes are inactive.

Arguments in angle brackets are placeholders. Use a dot for decimal values.

## LAB commands

These work separately from Research console integration. Enter a command **without arguments** to query its current value and help.

| Command | Arguments |
| --- | --- |
| `labplus_speedometer_show` | `0\|1 [current\|landing\|vertical]` |
| `labplus_speedometer_showlanding` | `0\|1` |
| `labplus_speedometer_showvertical` | `0\|1` |
| `labplus_speedometer_verticalsign` | `0\|1`; signed up/down or magnitude |
| `labplus_speedometer_x_pos` | `<pixels> [current\|landing\|vertical]` |
| `labplus_speedometer_y_pos` | `<pixels> [current\|landing\|vertical]` |
| `labplus_speedometer_size` | `<0.1..10> [current\|landing\|vertical]` |
| `labplus_speedometer_color` | `<R> <G> <B> [current\|landing\|vertical]`; each 0..255 |
| `labplus_lowhptintopacity` | `<0..100>`; 100 is the original opacity |

For position, size and color, omitting the target changes all three meters. The untargeted visibility command controls the master switch. Lighting, input protection and driver-line hiding are configured on [LAB](LAB).

The original game commands `noclip`, `notarget` and `god` remain unchanged; argument-free toggles are reflected in the cheat display.

## Research HUD actions

These require the compatible loaded meathook/action bridge. The new groups are independent and start off on fresh settings.

| Command | Arguments | Display |
| --- | --- | --- |
| `labplus_enemies` | `0\|1` | Living enemies by active encounter |
| `labplus_enemy_hp_bars` | `0\|1` | World HP/GK panels, including standalone living AI |
| `labplus_timers` | `0\|1` | Equipment, powerup, Sticky and supported animation timers |
| `labplus_techs` | `0\|1` | BATS and Fast DBlade observations |
| `labplus_debughud_scale` | `<0.5..3>` | Shared Research HUD text scale; no argument queries it |

Use `com_debugHUD 1` for the game's Debug HUD visibility. See [Invisible numbers](Invisible-numbers) for state meanings and timer scope.

## Viewer/Research short actions

These take an explicit value even when native cvar creation is unavailable.

| Action | Values |
| --- | --- |
| `labplus_viewer` | 0/1, viewer master |
| `labplus_triggers`, `labplus_collision`, `labplus_walls` | 0/1, geometry groups |
| `labplus_inspector`, `labplus_disabled`, `labplus_history` | 0/1 |
| `labplus_path`, `labplus_player`, `labplus_surface`, `labplus_attempts` | 0/1 |
| `labplus_encounters` | 0/1, encounter HUD; no argument lists managers |
| `labplus_distance` | 20..200 metres |
| `labplus_fill` | 0 outlines, 1 filled |

## Persistent Research cvars

These cvars are currently available for **6.66 Rev 3 only**. The other command groups above have separate prerequisites. On other builds, use the UI or an available short action; do not invent unsupported cvars with `set`.

| Cvar | Range | Fresh value |
| --- | --- | --- |
| `labplus_viewer_show` | 0/1 | 0 |
| `labplus_triggers_show` | 0/1 | 1 |
| `labplus_collision_show` | 0/1 | 1 |
| `labplus_walls_show` | 0/1 | 1 |
| `labplus_inspector_show` | 0/1 | 0 |
| `labplus_disabled_volumes_show` | 0/1 | 0 |
| `labplus_volume_history_show` | 0/1 | 0 |
| `labplus_path_show` | 0/1 | 1 |
| `labplus_player_show` | 0/1 | 0 |
| `labplus_surface_show` | 0/1 | 0 |
| `labplus_attempts_show` | 0/1 | 1 |
| `labplus_encounters_show` | 0/1 | 0 |
| `labplus_viewer_distance` | 20..200 | 40 |
| `labplus_viewer_fill` | 0 outlines / 1 filled | 1 |

Saved preferences replace fresh values. Where native cvars are available, entering a cvar name without a value queries the engine variable.

## Recording and other actions

| Command | Arguments | Action |
| --- | --- | --- |
| `labplus_commands_list` | None | List available controls |
| `labplus_encounters_list` | None | Print observed active managers |
| `labplus_recording_start` | None | Start/resume the research journal |
| `labplus_recording_stop` | None | Stop the research journal |
| `labplus_recording_clear` | None | Begin a new journal; older files remain |
| `labplus_markers_add` | `<text>` | Add a marker |
| `labplus_trigger_set` | `<exact-class> 0\|1` | Change one recognized trigger class |

Aliases: `labplus_help`, `labplus_record 1` / `labplus_record 0`, `labplus_clear`, `labplus_mark <text>` and `labplus_trigger <class> 0|1`.

Use the Types window for hierarchical path filters. Timeline commands do not start/stop Movement Lab named attempts.

## Example

```text
labplus_commands_list
labplus_viewer 1
labplus_collision 0
labplus_triggers 1
labplus_distance 60
labplus_inspector 1
labplus_enemy_hp_bars 1
labplus_timers 1
labplus_debughud_scale 1.25
labplus_recording_start
labplus_markers_add before arena entry
labplus_recording_stop
```

Names may remain in game completion after the companion closes, but their companion handlers are inactive. Reopen LAB Plus for the matching process. See [Troubleshooting](Troubleshooting) for unknown commands or unavailable native cvars.
