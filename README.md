# LAB Plus

**Explore DOOM Eternal beyond what you can normally see.**

LAB Plus is a free Windows tool for DOOM Eternal that helps you explore levels, visualize hidden geometry, track movement, analyze combat and practice speedrunning.

Everything in one customizable interface.

[**Download Latest Release**](https://github.com/IlyaTsyganov/LAB-Plus/releases/latest) • [**Wiki**](https://github.com/IlyaTsyganov/LAB-Plus/wiki) • [**Report an Issue**](https://github.com/IlyaTsyganov/LAB-Plus/issues)

## Features

- **Collision Viewer** — See collision geometry, triggers and invisible walls directly in the game.
- **Movement Tools** — Track your speed, visualize movement and practice routes.
- **Combat Information** — Monitor enemy health, active encounters, equipment cooldowns and other useful information.
- **Level Exploration** — Inspect hidden objects, record paths and compare level geometry.
- **Checkpoint Manager** — Organize and reload supported checkpoints for easier practice.
- **Visual Customization** — Adjust overlay colors, visibility and other visual settings.

LAB Plus supports multiple versions of DOOM Eternal. Feature availability may vary depending on the game version.

### Console toggle binds

Binary settings use the engine's normal cvar toggle syntax:

```text
bind x "toggle labplus_viewer_show"
bind f "toggle labplus_viewer_fill"
bind v "toggle labplus_speedometer_sign_vertical"
bind r "toggle labplus_record"
```

All 25 existing binary controls are exposed, including Filled/Outlines mode,
research recording and the four binary speedometer controls. Use the preferred
names printed by `labplus_commands_list`. Short binary setting names such as
`labplus_viewer` are synchronized cvar aliases, so `bind x "toggle labplus_viewer"`
also works. `labplus_encounters` retains its encounter-list command behavior;
use `labplus_encounters_show` to toggle that HUD.
See the [complete list of binary cvars and short aliases](docs/console-toggle-binds.md).

Native cvar creation is audited for all 22 supported retail builds, including
1.1. A guarded command fallback remains for unavailable or modified ABIs and
accepts explicit setting values. The list describes each setting and its range.
For native cvars, `toggle` is a separate engine command placed before the cvar
name; cvar value completion offers `0` and `1`.
See [the cvar ABI audit](docs/validation/cvar-factory-profiles.json).

## Download & Installation

1. Download the latest Windows release from the [Releases page](https://github.com/IlyaTsyganov/LAB-Plus/releases/latest).
2. Extract the ZIP archive into a folder.
3. Run `LABPlus.exe`.
4. Follow the [installation guide](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Installation-and-first-launch) to complete the initial setup and launch DOOM Eternal through LAB Plus.

**Requirements:** Windows 64-bit, .NET 10 Desktop Runtime (x64) and Microsoft Edge WebView2 Runtime.

### Important: Internet Access Restriction

LAB Plus requires an enabled Windows Firewall rule that blocks DOOM Eternal from accessing the internet. **Without this rule, LAB Plus will refuse to launch the game.**

This restriction is intentional. It is designed to discourage misuse of the software in online or competitive environments and to help ensure that LAB Plus remains a tool for offline research, learning and speedrunning practice.

The firewall rule can be configured using DESRU. See the [installation guide](https://github.com/IlyaTsyganov/LAB-Plus/wiki/Installation-and-first-launch) for instructions.

## Need Help?

Visit the [Wiki](https://github.com/IlyaTsyganov/LAB-Plus/wiki) for detailed instructions, feature explanations and troubleshooting.

Found a bug or have a suggestion? Open an [issue on GitHub](https://github.com/IlyaTsyganov/LAB-Plus/issues).

## Development checks

Every pull request, including a draft and a pull request into a refactoring
branch, runs **Pull request checks / Windows build and fixtures**. A new commit
cancels the previous run for that pull request. The workflow builds both native
libraries, runs fixtures and validates the runtime package. Its GitHub token has
read-only access; it does not create tags or publish releases.

To reproduce the checks on Windows x64, install the .NET 10 SDK and Visual Studio
with the Desktop development with C++ workload and Windows SDK. Run these commands
in PowerShell from the repository root:

```powershell
cmd /d /c .\Native\CollisionViewer\build-viewer.cmd
.\Native\Lighting\build.ps1
.\Native\CollisionViewer\tests\run-fixtures.ps1
dotnet run --project Tests/Updates/Updates.Tests.csproj --configuration Release
dotnet run --project Tests/Launcher/LauncherTests.csproj --configuration Release
dotnet run --project Tests/ScreenState/ScreenState.Tests.csproj --configuration Release
dotnet run --project Tests/Hud/Hud.Tests.csproj --configuration Release
dotnet publish labplus.csproj --configuration Release --self-contained false -p:PlatformTarget=x64 -p:PublishSingleFile=false -p:DebugType=None -p:DebugSymbols=false --output build/pr-checks
.\package-local.ps1 -Source build/pr-checks
```

Stop if any command fails. Native test sources use the `*_tests.cpp` suffix;
automated fixture executables use `*Tests.exe`. The fixture runner checks that all
test sources are included in the MSVC build and rejects missing or stale binaries.
Add new suites to both native build scripts.

The screen state fixtures check that removed controls and tree nodes can be
collected, stale input is ignored, and hidden panels, inactive tabs and collapsed
branches keep their state. They run without opening a browser or a game.

Launcher fixtures also check DLL identity caching. Status polling reuses the
cached name while size, write time and creation time are unchanged. Launch
preparation always re-hashes a read-locked source before copying its versioned
runtime DLL, including replacements that preserve those metadata values.

These fixtures do not require DOOM Eternal. Visual interface, GPU and live-game
acceptance are separate checks; `build-local.ps1` also runs interface previews.
The release workflow remains a separate process.

## Purpose & Disclaimer

LAB Plus is developed exclusively for educational, research and practice purposes, with a particular focus on understanding DOOM Eternal's game mechanics and improving speedrunning knowledge and techniques.

The software is not intended to provide an unfair advantage over other players, facilitate cheating in competitive environments or interfere with the experience of other users.

LAB Plus is an independent, unofficial community project. The author does not claim ownership of DOOM Eternal or any associated trademarks, copyrights, game assets or other intellectual property. All respective rights remain with their original owners. No infringement of third-party rights is intended.

LAB Plus is not affiliated with, sponsored by or endorsed by id Software, Bethesda Softworks or their respective parent companies.

**Use at your own risk.** The software is provided "as is", without warranties of any kind. The author assumes no responsibility for crashes, data loss, game instability, account restrictions or other consequences arising from its use, to the extent permitted by applicable law.

Users are responsible for complying with applicable game and platform terms of service, as well as the rules of any speedrunning communities or events in which they participate.

---

Licensed under [GNU GPL v3](LICENSE).
