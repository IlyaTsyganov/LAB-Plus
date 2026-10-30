# Troubleshooting

| Symptom | What to check |
| --- | --- |
| Application will not start | Install .NET **10 Desktop Runtime x64**, extract the entire ZIP and retain the folder structure. |
| Empty or failed desktop view | Install/update WebView2 Evergreen Runtime; confirm packaged Assets/fonts/UI files are present and writable data storage is available. |
| Launch is blocked | Check the exact selected EXE and effective outbound Windows Firewall coverage for every active profile. Disabled, inbound, allow or partial rules do not satisfy the gate. LAB Plus does not change firewall rules. |
| Viewer enabled but no geometry | Fully restart through LAB Plus after enabling/updating the layer. Confirm a recognized executable, loaded level and valid camera/physics state. Inspect `viewer-runtime/VulkanViewer.log`. |
| Overlay hidden in menus/pause | This is intentional; test in active gameplay. The console also clips overlay drawing. |
| HUD text absent | A compatible meathook must already be loaded. Profile fixtures do not verify live fonts or callback visibility. Review HUD diagnostics and restart with the current layer. |
| Unknown console command | Use `labplus_`, keep LAB Plus running with the matching EXE selected and confirm meathook/engine command integration. Old prefixes are inactive. |
| Native cvars unavailable | Native constructor/register/set creation is audited only for 6.66 Rev 3. Other versions withhold it. Do not use a made-up `set` command to create cvars. |
| Commands listed but do nothing after closing LAB Plus | Registered names can remain in the game's completion list; handlers refuse companion actions when the companion is absent. Reopen it for the matching process. |
| Lighting fix absent | Only NVIDIA and recognized 1.0/1.1/2.0 launches through LAB Plus opt in. Keep Steam running; inspect `Lighting fix: on` and `Lighting fix: patched ...` log lines. |
| Gaps in routes/measurements | Menu/pause, unreadable data, jumps and timing gaps intentionally break lines. Missing samples are not interpolated evidence. |
| Save/Load unavailable | Confirm 6.66 Rev 3 / TAG1 / slot 3, same account, Cloud off, paused game and valid Steam/process/session checks. Offline tools instead require both Steam and game closed. |
| Restore interrupted | Preserve all originals and retained plan/evidence paths. Follow the exact restore plan; do not merge partial directories. |

## Reporting a problem

Open an [issue](https://github.com/IlyaTsyganov/LAB-Plus/issues) with the LAB Plus version, game version/profile, GPU/driver, exact steps and the relevant log excerpt. Remove account identifiers, personal paths and unrelated data. Do not attach checkpoint archives or entire local-data folders unless you have reviewed them and deliberately intend to share that data.

The current release includes experimental research integration. A passing automated fixture or a Vulkan draw-call count alone does not prove visible in-game rendering, script execution or feature support on an unrecognized build.
