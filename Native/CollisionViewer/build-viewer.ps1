param([string]$Compiler = $env:LABPLUS_CXX, [switch]$RunTests)
$ErrorActionPreference = 'Stop'
$nativeDirectory = $PSScriptRoot
if (!$Compiler) {
    $command = Get-Command clang++ -ErrorAction SilentlyContinue
    if ($command) { $Compiler = $command.Source }
}
if (!$Compiler -or !(Test-Path -LiteralPath $Compiler)) { throw 'Provide the LLVM-MinGW clang++.exe path with -Compiler, or use build-viewer.cmd with MSVC.' }
$headers = Join-Path $nativeDirectory '..\..\third_party\Vulkan-Headers\include'
$output = Join-Path $nativeDirectory 'bin'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$flags = @('-std=c++17', '-fms-extensions', '-D_WIN32_WINNT=0x0A00', '-DNTDDI_VERSION=0x0A000000', '-O2', '-static', "-I$headers")
& $Compiler @flags -shared (Join-Path $nativeDirectory 'src\vulkan_layer.cpp') -o (Join-Path $output 'labplus-layer.dll') -luser32 -lbcrypt
if ($LASTEXITCODE -ne 0) { throw 'Native layer compilation failed.' }
$suites = [ordered]@{
 'fun_mode_tests.cpp'='FunModeTests';
 'app_lifetime_tests.cpp'='AppLifetimeTests';
 'bats_weapon_tests.cpp'='BatsWeaponTests';
 'health_bar_render_tests.cpp'='HealthBarRenderTests';
 'speedrun_tech_tests.cpp'='SpeedrunTechTests';
 'enemy_telemetry_tests.cpp'='EnemyTelemetryTests'; 'game_timer_tests.cpp'='GameTimerTests'; 'invisible_numbers_tests.cpp'='InvisibleNumbersTests';
 'debug_hud_stack_tests.cpp'='DebugHudStackTests';
 'debug_hud_render_tests.cpp'='DebugHudRenderTests';
 'sync_interaction_tests.cpp'='SyncInteractionTests';
 'performance_tests.cpp'='PerformanceTests';
 'appearance_tests.cpp'='AppearanceTests'; 'practice_live_tests.cpp'='PracticeLiveTests'; 'viewer_behavior_tests.cpp'='ViewerBehaviorTests';
 'depth_tracker_tests.cpp'='DepthTrackerTests'; 'inspector_tests.cpp'='InspectorTests';
 'research_tests.cpp'='ResearchTests'; 'movement_tests.cpp'='MovementTests';
 'level_state_tests.cpp'='LevelStateTests'; 'encounter_tests.cpp'='EncounterTests';
 'entity_graph_tests.cpp'='EntityGraphTests'; 'labplus_cvar_tests.cpp'='LabCvarTests';
 'lighting_fix_tests.cpp'='LightingFixTests'; 'layer_loader_probe.cpp'='LayerLoaderProbe';
 'patch_port_tests.cpp'='RetailPatchPortProbe'; 'retail_profile_probe.cpp'='RetailProfileProbe'; 'vulkan_smoke.cpp'='VulkanSmoke'
}
foreach ($entry in $suites.GetEnumerator()) {
    $source = Join-Path $nativeDirectory "tests\$($entry.Key)"
    $target = Join-Path $output "$($entry.Value).exe"
    $entryFlags = @()
    if ((Get-Content -LiteralPath $source -Raw) -match '\bwmain\s*\(') { $entryFlags += '-municode' }
    Write-Output "Building $($entry.Value)"
    & $Compiler @flags @entryFlags $source -o $target -luser32 -lbcrypt
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($entry.Value)" }
    if ($RunTests -and $entry.Value.EndsWith('Tests')) {
        & $target
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $($entry.Value)" }
    }
}
Write-Output 'LAB Plus native build completed.'
