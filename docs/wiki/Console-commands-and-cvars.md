# Console commands and cvars

## Prerequisites

Keep LAB Plus running with the matching game executable selected. The current layer must have loaded at full game startup. Console registration uses a compatible, already-loaded meathook with validated engine callbacks. Native cvar construction/register/set is audited for **6.66 Rev 3 only** and still awaits in-game acceptance with the rebranded layer. Unsupported versions withhold creation; do not invent variables with `set`.

All active controls start with **`labplus_`**. Names from the previous product are not registered. Arguments shown in angle brackets are placeholders; replace them instead of typing brackets.

## Persistent state cvars

Use `name value` to set a cvar. Where native cvars are available, entering its name without a value queries the engine variable. Boolean values are 0/1. Initial values below apply to fresh settings; saved preferences are loaded on later runs. UI changes and owned cvars synchronize through the companion.

| Cvar | Meaning | Range | Initial | Short action alias |
| --- | --- | --- | --- | --- |
| `labplus_viewer_show` | Master viewer | 0–1 | 0 | `labplus_viewer` |
| `labplus_triggers_show` | Trigger geometry | 0–1 | 1 | `labplus_triggers` |
| `labplus_collision_show` | World collision | 0–1 | 1 | `labplus_collision` |
| `labplus_walls_show` | Invisible walls | 0–1 | 1 | `labplus_walls` |
| `labplus_inspector_show` | Crosshair inspector | 0–1 | 0 | `labplus_inspector` |
| `labplus_disabled_volumes_show` | Disabled volume outlines | 0–1 | 0 | `labplus_disabled` |
| `labplus_volume_history_show` | Recent volume history | 0–1 | 0 | `labplus_history` |
| `labplus_path_show` | Research path | 0–1 | 1 | `labplus_path` |
| `labplus_player_show` | Player collider | 0–1 | 0 | `labplus_player` |
| `labplus_surface_show` | Support-angle HUD | 0–1 | 0 | `labplus_surface` |
| `labplus_attempts_show` | Movement attempt overlays | 0–1 | 1 | `labplus_attempts` |
| `labplus_encounters_show` | Active encounter HUD | 0–1 | 0 | `labplus_encounters` |
| `labplus_viewer_distance` | View distance in metres | 20–200 | 40 | `labplus_distance` |
| `labplus_viewer_fill` | 0 = outlines; 1 = filled | 0–1 | 1 | `labplus_fill` |

The short aliases take an explicit value; their no-argument form is not a general cvar query. `labplus_encounters` is the special case: no argument lists managers; 0/1 toggles its HUD.

## Action commands

| Command | Arguments | Action |
| --- | --- | --- |
| `labplus_commands_list` | None | List available cvars and actions |
| `labplus_encounters_list` | None | Print observed active encounter managers/reference names |
| `labplus_recording_start` | None | Start/resume the research timeline journal |
| `labplus_recording_stop` | None | Stop the research timeline journal |
| `labplus_recording_clear` | None | Begin a new research session; older session files remain |
| `labplus_markers_add` | `<text>` | Add a marker to the research journal |
| `labplus_trigger_set` | `<exact-class> 0\|1` | Change one recognized exact trigger class |

Marker text is cleaned/bounded and the complete IPC request is limited to 2,000 bytes. A trigger class must be valid and recognized. Use **Trigger types** for nested folder filters; the console class action does not set path prefixes.

## Additional compatibility aliases

| Alias | Equivalent |
| --- | --- |
| `labplus_help` | `labplus_commands_list` |
| `labplus_encounters` | No arguments: list; with 0/1: HUD switch |
| `labplus_record 1` / `labplus_record 0` | Start/resume / stop timeline |
| `labplus_clear` | `labplus_recording_clear` |
| `labplus_mark <text>` | `labplus_markers_add <text>` |
| `labplus_trigger <class> 0\|1` | `labplus_trigger_set <class> 0\|1` |

All other short state aliases are listed alongside their cvars above. They retain the `labplus_` prefix. Timeline recording controls do not start or stop named Movement Lab attempts; use that tool's own recording controls.

## Examples

```text
labplus_commands_list
labplus_viewer_show 1
labplus_collision_show 0
labplus_triggers_show 1
labplus_viewer_distance 60
labplus_viewer_fill 0
labplus_inspector_show 1
labplus_recording_start
labplus_markers_add route A: before entering arena
labplus_encounters_list
labplus_recording_stop
```

If native cvars are unavailable but the action bridge works, use short actions such as `labplus_distance 60` or change the same preference in the interface. A new/first layer enable still requires a full game restart.

## Lifetime and diagnostics

Requests are same-user and scoped to the selected game PID. The bridge accepts only known validated actions; it does not forward arbitrary shell/engine commands. Names can remain in completion after the companion closes, but handlers refuse companion actions and changes cannot be persisted through the absent companion. The registered module remains pinned rather than globally unregistering names from the engine.

Foreign ownership or failed ABI validation blocks native cvar registration with a diagnostic instead of repeated timed registration. Look in `viewer-runtime/VulkanViewer.log`. A successful fixture or completion entry does not prove native HUD visibility or live cvar acceptance on an unsupported executable.
