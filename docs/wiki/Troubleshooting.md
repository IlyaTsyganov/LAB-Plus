# Troubleshooting

Start with the exact LAB Plus version, selected game executable and a full game restart after a native-layer update. Check [Compatibility](Compatibility) before treating Unavailable as a malfunction.

## Startup and launch

| Symptom | Check |
| --- | --- |
| LABPlus.exe will not start | Install .NET **10 Desktop Runtime x64** and extract the complete ZIP. |
| Empty/failed desktop interface | Install or update WebView2 Evergreen; retain packaged Assets/UI/fonts and writable data storage. |
| Launch blocked | Select the exact EXE and enable its effective outbound firewall block on all active profiles. Inbound, allow, disabled or another-version rules do not qualify. |
| Firewall service/state error | Windows Firewall and Base Filtering Engine must be running and readable. LAB Plus does not change their settings. |
| Update/restart pending | Fully close the game and relaunch through the matching new package; returning to the menu does not reload a DLL. |

The missing-rule message is:

```text
Launch blocked: enable firewall rule for the selected DOOM Eternal version(e.g. via DESRU).
```

## Viewer and HUD

| Symptom | Check |
| --- | --- |
| Viewer enabled, no geometry | Current layer loaded at startup, recognized executable, loaded level, geometry toggles, filters and distance. Inspect the native log. |
| Overlay absent in pause/menu or clipped by console | Intentional visibility behavior; test during gameplay. |
| No Research metadata/HUD | Compatible meathook must already be loaded. Its support is separate from collision-profile recognition. |
| No enemy HP panels | Enable **Enemy HP bars** specifically; the enemy table/HUD switch is separate. Restart if the layer was not loaded. The target needs a valid live position and may be occluded by walls. |
| HUD is crowded | Hide unused groups or use `labplus_debughud_scale 0.8`. The window remains useful for detailed lists. |
| Colors belong to the wrong group | Check the separate World/Triggers/Walls preferences; preserve custom colors rather than resetting everything. |
| Slow rendering | Begin at 40 m, lower distance or hide unnecessary groups. A synthetic CPU improvement is not a guarantee of an FPS increase on every scene. |

## Observations and commands

- **Unavailable timer/tech:** read its detail in Invisible numbers. Unsupported build, clock, ownership, animation or BATS configuration must not become a guessed zero.
- **Equipment missing from the compact HUD:** unacquired/inactive/unavailable rows can be omitted there; inspect the window. Initialized counters do not prove acquisition.
- **Several Sticky charge countdowns match:** expected during an empty-magazine reload; all missing charges return together.
- **Interaction timer ends differently from control lock:** most rows are current-clip timers. Only the verified Praetor Suit Point asset on game 1.1 has Player lock semantics.
- **Changed cvar list empty:** ordinary menu/profile settings are excluded, as are initial nondefault session preferences. Eligible changed values are separate from active noclip/notarget/god states.
- **A cvar you did not type appears:** the game or another tool can change gameplay values. The list reports state, not who caused it.
- **Unknown command:** use `labplus_` and keep the matching companion/layer/bridge active. LAB commands and Research commands have different prerequisites.
- **Native cvars unavailable:** construction is audited only for 6.66 Rev 3; try a supported short action or the UI. Do not create variables with invented `set` commands.
- **Completion lists a command after LAB Plus closes:** names can remain, but companion actions are inactive.

Lighting Fix intentionally stays off on AMD, Intel, unknown GPUs and versions other than verified NVIDIA 1.0/1.1/2.0.

## Checkpoints and recordings

Live checkpoint loading requires 6.66 Rev 3 / TAG1 / slot 3, the same account, Cloud off and all runtime checks. Offline restore instead requires both Steam and game closed. For an interrupted restore, preserve originals and retained plan paths; follow [Checkpoints and recovery](Checkpoints-and-recovery).

Route gaps can reflect pause, unreadable data or a jump. They are not filled with invented samples.

## Report a reproducible problem

Use [Issues](https://github.com/IlyaTsyganov/LAB-Plus/issues) with application version, game build, GPU/driver, enabled features, exact steps and relevant log excerpts. Locations are in [Data and migration](Data-and-migration). Review personal paths/account data before sharing.

Describe the actions you performed and what actually appeared on screen. Support is specific to each feature and game build.
