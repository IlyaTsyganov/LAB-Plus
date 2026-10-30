param([string]$NativeCompiler = $env:LABPLUS_CXX, [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $env:DOTNET_CLI_UI_LANGUAGE = 'en'
    $dotnet = Join-Path $PSScriptRoot '..\.tools\dotnet\dotnet.exe'
    if (!(Test-Path -LiteralPath $dotnet)) { $dotnet = (Get-Command dotnet -ErrorAction Stop).Source }
    if (!$NativeCompiler) {
        $portableCompiler = Join-Path $PSScriptRoot '..\.tools\llvm-mingw-20261020-ucrt-x86_64\bin\clang++.exe'
        if (Test-Path -LiteralPath $portableCompiler) { $NativeCompiler = $portableCompiler }
    }
    if ($NativeCompiler) { & '.\Native\CollisionViewer\build-viewer.ps1' -Compiler $NativeCompiler -RunTests:(!$SkipTests) }
    else {
        & cmd /c '.\Native\CollisionViewer\build-viewer.cmd'
        if ($LASTEXITCODE -ne 0) { throw 'Native MSVC build failed.' }
        if (!$SkipTests) {
            Get-ChildItem -LiteralPath '.\Native\CollisionViewer\bin' -Filter '*Tests.exe' | ForEach-Object { & $_.FullName; if ($LASTEXITCODE -ne 0) { throw "Native test failed: $($_.Name)" } }
        }
    }
    if (!$SkipTests) {
        & $dotnet run --project '.\Tests\Launcher\LauncherTests.csproj' -c Release
        if ($LASTEXITCODE -ne 0) { throw 'Launcher checks failed.' }
        New-Item -ItemType Directory -Path '.\build\screenshots' -Force | Out-Null
        & $dotnet run --project '.\Tests\InspectorPreview\InspectorPreview.csproj' -c Release -- (Join-Path $PSScriptRoot 'build\screenshots\application.png')
        if ($LASTEXITCODE -ne 0) { throw 'Interface checks failed.' }
        & $dotnet run --project '.\Tests\VisualPreview\VisualPreview.csproj' -c Release -- (Join-Path $PSScriptRoot 'build\visual-qa')
        if ($LASTEXITCODE -ne 0) { throw 'WebView interface checks failed.' }
    }
    & $dotnet publish '.\labplus.csproj' -c Release --self-contained false -o '.\publish'
    if ($LASTEXITCODE -ne 0) { throw 'Publishing failed.' }
    Write-Output 'Ready: publish\labplus.exe. Keep the entire publish folder together.'
}
finally { Pop-Location }
