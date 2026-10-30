# Building and validation

## Source layout

| Directory | Contents |
| --- | --- |
| `Configuration` | Preferences, trigger catalogues and import migration |
| `Game` | Launch, process readers, recording, graph data and checkpoint workflows |
| `UI/State` | Plain C# screen state and actions |
| `UI` | Screen controllers, presenter and the single-WebView native host |
| `Assets/ui` | Packaged Slayer Workspace HTML/CSS/JavaScript |
| `Assets/fonts` | Original Eternal typefaces |
| `Native/CollisionViewer` | Vulkan layer, readers, shaders, tests and technical records |
| `Tests` | Managed launcher, headless screen and actual WebView checks |
| `docs/wiki` | Versioned source for this Wiki |

## Build on Windows x64

Install the .NET 10 SDK and either Visual Studio C++ Build Tools or a compatible LLVM-MinGW toolchain. Vulkan-Headers are included under `third_party/Vulkan-Headers`; restore the pinned WebView2 NuGet package using the project configuration.

```powershell
./build-local.ps1
# or use an explicit portable compiler
./build-local.ps1 -NativeCompiler C:\tools\llvm-mingw\bin\clang++.exe
```

The script finds a task-local SDK/compiler when available, builds the native layer, runs isolated fixtures and managed/UI checks and publishes to `publish`. `-SkipTests` is intended for a subsequent build after validation. The original MSVC native route remains available:

```powershell
cmd /c .\Native\CollisionViewer\build-viewer.cmd
dotnet publish .\labplus.csproj -c Release --self-contained false -o .\publish
```

Publish the whole folder. Building only the managed project without first producing `Native/CollisionViewer/bin/labplus-layer.dll` does not produce a complete viewer package.

## Checks

```powershell
dotnet run --project Tests/Launcher/LauncherTests.csproj -c Release
dotnet run --project Tests/InspectorPreview/InspectorPreview.csproj -c Release -- build/screenshots/application.png
dotnet run --project Tests/VisualPreview/VisualPreview.csproj -c Release -- build/visual-qa
```

`Tests/InspectorPreview` exercises plain screen controllers despite its historical project name. `Tests/VisualPreview` tests actual WebView rendering, first-frame readiness and navigation. There is one native Control subclass: a host containing one WebView2. Old native fields, tables, buttons and trees are not instantiated behind the new interface.

The native build contains eleven fixture suites, plus loader/profile probes and the isolated Vulkan smoke program. `Native/CollisionViewer/tests/run-gpu-tests.ps1` exercises 19 GPU readback cases; `-LightingFix` runs the lighting variants. Split-queue cases require suitable hardware. An ASCII `-RuntimeDirectory` can avoid loader issues with non-ASCII temporary paths.

These tests do not establish live-game acceptance. The released native DLL was retained byte for byte from the verified local rebrand build; the final managed/UI migration reran its relevant checks. See [Compatibility](Compatibility) for precise acceptance limits.

## License and attribution

Source is [GPL-3.0](https://github.com/IlyaTsyganov/LAB-Plus/blob/main/LICENSE), retaining the original project history. Vulkan-Headers, the lighting correction and WebView2 retain their included licenses/notices. DOOM Eternal fonts/logo-derived assets remain property of their respective owners; inclusion does not transfer ownership or grant additional rights. LAB Plus is not affiliated with or endorsed by id Software or Bethesda.
