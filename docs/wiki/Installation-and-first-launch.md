# Installation and first launch

## Requirements

- Windows x64 and a [recognized DOOM Eternal executable](Compatibility).
- [.NET 10 Desktop Runtime x64](https://dotnet.microsoft.com/en-us/download/dotnet/10.0), rather than only the base .NET runtime.
- [Microsoft Edge WebView2 Evergreen Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/).
- A working Vulkan driver.
- For Research metadata, Research HUD and its console commands: a compatible, already-loaded meathook. It is not included in the package.

## Install

1. Download the Windows x64 ZIP from [Releases](https://github.com/IlyaTsyganov/LAB-Plus/releases/latest).
2. If a checksum file is supplied, compare it with `Get-FileHash .\your-downloaded-file.zip -Algorithm SHA256`.
3. Extract **all files** to a writable folder. Preserve the relative locations of Assets, Native, runtimes, libraries and licenses.
4. Run **LABPlus.exe**. Do not copy the EXE out of its package folder.

The desktop interface uses packaged local assets. Rendering it does not require an internet connection.

## Select and launch the game

1. On **Viewer**, use the folder button beside **Launch game** to select the actual `DOOMEternalx64vk.exe`.
2. Enable an outbound Windows Firewall block for that exact executable, for example through DESRU.
3. Enable the features you want. Geometry is configured on Viewer; speedometers and display fixes are on LAB; additional observations are on Research.
4. Launch through LAB Plus and load a level. Keep the companion running.

The launch check applies even if Collision Viewer and all other optional layers are off. A rule for another installed version, an inbound rule or a disabled rule is insufficient. LAB Plus checks firewall state and does not create or change rules.

If the rule is missing, the message is:

```text
Launch blocked: enable firewall rule for the selected DOOM Eternal version(e.g. via DESRU).
```

The launcher also needs Windows Firewall and Base Filtering Engine services and effective coverage for the active network profiles.

## Updating and restarting

**Fully exit the game** before first loading a newly enabled native feature or replacing its DLL. Returning to the menu does not reload the layer. Install the complete new package, run its LABPlus.exe and relaunch the selected game.

Once the correct layer is loaded, display preferences can update while the game runs. A restart/update-pending message means the running game still needs the matching layer.

Lighting Fix only applies to verified 1.0, 1.1 and 2.0 on NVIDIA. It stays off on AMD, Intel, unknown hardware and other game versions; see [Compatibility](Compatibility).

See [LAB](LAB), [Invisible numbers](Invisible-numbers) or [Troubleshooting](Troubleshooting) for the next step.
