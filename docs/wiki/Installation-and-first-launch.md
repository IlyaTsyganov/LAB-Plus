# Installation and first launch

## Requirements

- Windows x64 and a supported `DOOMEternalx64vk.exe`.
- [.NET 10 Desktop Runtime x64](https://dotnet.microsoft.com/en-us/download/dotnet/10.0). Choose **Desktop Runtime**, not only the base .NET runtime.
- [Microsoft Edge WebView2 Evergreen Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/). The desktop interface renders local packaged assets through WebView2.
- A working Vulkan driver. Optional native HUD text and console integration additionally require a compatible meathook; it is not bundled.

## Install

1. Download `labplus-0.8.0-win-x64.zip` and `SHA256SUMS.txt` from the release.
2. Optionally verify the ZIP with `Get-FileHash .\labplus-0.8.0-win-x64.zip -Algorithm SHA256` and compare with the checksum file.
3. Extract **all** files to a writable folder. Keep `Assets`, `Native`, `runtimes`, licenses and the managed libraries beside `labplus.exe` in their original relative locations.
4. Run `labplus.exe`. The release is framework dependent; it does not bundle the .NET or WebView2 runtimes.

## Configure and launch

1. Open **Settings** and select the actual game executable. LAB Plus reads its PE identity and code fingerprint; a folder name is not a version check.
2. Configure the matching outbound block in DESRU. The **Launch game** button requires an effective Windows Firewall block for this exact EXE on every active network profile. LAB Plus checks the rules; it does not create or alter them.
3. On **Viewer**, enable Collision Viewer and choose geometry, fill style and distance.
4. Launch the game through LAB Plus. It passes `+com_skipIntroVideo 1`.
5. Load a level. Keep LAB Plus open to use console actions, recording and the research tools.

Enabling the layer for the first time or updating its DLL requires a **full game restart**. Scalar options can update in a running process once the appropriate layer is already loaded. Merely returning to the main menu does not reload a DLL.

## Navigation

**Viewer** contains geometry and inspector controls. **Research** opens the timeline, Movement Lab, entity links, level comparison, encounters and research points. **Saves** contains the checkpoint library. **Settings** contains executable selection, companion tools, logs and help.

The interface uses only local HTML/CSS, fonts and images; it does not require an internet connection for rendering. Standard file and folder pickers are Windows dialogs. `labplus.exe --preview` opens an isolated interface preview without layer registration or the console-command listener.

See [Troubleshooting](Troubleshooting) if startup, firewall checks or rendering fail.
