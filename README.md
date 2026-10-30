# LAB Plus

Windows research and practice companion for **DOOM Eternal**, featuring the **Slayer Workspace** interface, original Eternal typefaces and the red/gold Plus emblem. The interface is fully English; technical identifiers and console controls use `labplus` / `labplus_`.

[Download Windows x64](https://github.com/IlyaTsyganov/LAB-Plus/releases/latest) · [Detailed Wiki](https://github.com/IlyaTsyganov/LAB-Plus/wiki) · [Console reference](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Console-commands-and-cvars)

![Slayer Workspace](docs/images/workspace.png)

## Features

- Collision viewer: independent world collision, triggers and invisible walls; filled/outline style; 20–200 m distance.
- Persistent trigger-class and hierarchical folder filters, crosshair volume inspector, disabled-volume outlines and recent history.
- Research timeline: manual recording, in-game path, markers, search and complete CSV export.
- Movement Lab: named attempts, manual outcomes, up to six route overlays, player collider, velocity, target distances and surface projections.
- Entity links: navigable observed-reference graphs with fit, zoom, pan and JSON export.
- Before/after level comparison, changed-volume highlights and CSV/JSON export.
- Active encounter observations and optional native HUD.
- Research points with physical/camera data, categories, notes and copyable return data.
- Checkpoint library plus authenticated same-account archives, reviewed restore and backup recovery.
- Scoped Vulkan launch and automatic early-version NVIDIA lighting correction.

## Install and run

Extract the **entire** release ZIP and run `labplus.exe`. Keep every included file/folder together. Install **.NET 10 Desktop Runtime x64** and **Microsoft Edge WebView2 Evergreen Runtime**. Select `DOOMEternalx64vk.exe` in Settings, configure the matching outbound firewall block in DESRU, enable the viewer and launch the game through LAB Plus. The launch gate reads firewall state without changing it. First enable or a DLL update requires a full game restart.

See [installation](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Installation-and-first-launch), [compatibility](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Compatibility) and [troubleshooting](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Troubleshooting).

## Compatibility and scope

The collision reader recognizes 22 audited executable profiles from 1.0 through 6.66 Rev 3.2. Native HUD/console integration requires a compatible, already-loaded meathook, which is not bundled. Native cvar creation is audited for **6.66 Rev 3 only**. Live checkpoint loading is limited to **6.66 Rev 3 / TAG1 / slot 3**, same account, Cloud off and all runtime checks passing. These are completed autosaves, not arbitrary RAM snapshots.

Automated fixtures and isolated GPU tests do not prove every feature in a live game. The latest rebranded layer, encounter HUD and corrected native cvar creation still require in-game acceptance after restarting. The final UI migration passed managed, encrypted checkpoint-fixture, headless screen and packaged WebView checks; its native DLL is byte-identical to the earlier verified local build (11 native suites / 22 profiles / 38 GPU cases).

Collision decoding does not modify physics. The Vulkan loader loads the scoped layer at game startup; optional research integration uses engine APIs and the explicit checkpoint workflow can request an engine reload. LAB Plus is an unofficial community tool.

## Build

Requires Windows x64, the .NET 10 SDK and MSVC C++ Build Tools or LLVM-MinGW.

```powershell
./build-local.ps1
# Optional explicit portable native compiler:
./build-local.ps1 -NativeCompiler C:\tools\llvm-mingw\bin\clang++.exe
```

This builds the native layer, runs fixtures and managed/UI checks, and publishes the complete application to `publish`. See [Building and validation](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Building-and-validation) for individual commands and test boundaries. Versioned Wiki sources are in [`docs/wiki`](docs/wiki).

The UI uses packaged HTML/CSS via WebView2. Screen state/actions are plain C# objects; the native host contains only one WebView2 and does not create the former native widget layouts. Windows file/folder pickers remain OS dialogs. `labplus.exe --preview` opens an isolated UI preview.

Data lives under `%LOCALAPPDATA%\labplus`; `LABPLUS_DATA_DIRECTORY` accepts an absolute override. First-use migration copies previous preferences and archives without modifying originals or overwriting existing LAB Plus data. See [Data and migration](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Data-and-migration).

## License and attribution

Source is [GNU GPL v3](LICENSE), retaining history from [the original project](https://github.com/IlyaTsyganov/DEVIL). Vulkan-Headers, the [lighting correction](https://github.com/billymooreq/DoomEternal-v1.1--RTXFix) and WebView2 retain their included notices/licenses. DOOM Eternal typefaces and logo-derived assets remain the property of their respective owners; inclusion does not transfer ownership or grant additional rights. LAB Plus is not affiliated with or endorsed by id Software or Bethesda.
