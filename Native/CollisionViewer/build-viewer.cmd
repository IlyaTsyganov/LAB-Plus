@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>&1
if not errorlevel 1 goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio Installer was not found. Install Visual Studio with the C++ desktop workload.
  exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS (
  echo Visual Studio C++ build tools were not found.
  exit /b 1
)
call "%VCVARS%" >nul
if errorlevel 1 exit /b 1
:compiler_ready
set "VULKAN_HEADERS=..\..\third_party\Vulkan-Headers\include"
if not exist "%VULKAN_HEADERS%\vulkan\vulkan.h" (
  echo Vulkan-Headers must be available in the repository at third_party\Vulkan-Headers.
  exit /b 1
)
if not exist build mkdir build
if not exist bin mkdir bin
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /LD /I"%VULKAN_HEADERS%" src\vulkan_layer.cpp /Fo:build\vulkan_layer.obj /Fe:bin\labplus-layer.dll user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\practice_live_tests.cpp /Fo:build\practice_live_tests.obj /Fe:bin\PracticeLiveTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\viewer_behavior_tests.cpp /Fo:build\viewer_behavior_tests.obj /Fe:bin\ViewerBehaviorTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\depth_tracker_tests.cpp /Fo:build\depth_tracker_tests.obj /Fe:bin\DepthTrackerTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\inspector_tests.cpp /Fo:build\inspector_tests.obj /Fe:bin\InspectorTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\research_tests.cpp /Fo:build\research_tests.obj /Fe:bin\ResearchTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\movement_tests.cpp /Fo:build\movement_tests.obj /Fe:bin\MovementTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\level_state_tests.cpp /Fo:build\level_state_tests.obj /Fe:bin\LevelStateTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\encounter_tests.cpp /Fo:build\encounter_tests.obj /Fe:bin\EncounterTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\entity_graph_tests.cpp /Fo:build\entity_graph_tests.obj /Fe:bin\EntityGraphTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\labplus_cvar_tests.cpp /Fo:build\labplus_cvar_tests.obj /Fe:bin\LabCvarTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\lighting_fix_tests.cpp /Fo:build\lighting_fix_tests.obj /Fe:bin\LightingFixTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\layer_loader_probe.cpp /Fo:build\layer_loader_probe.obj /Fe:bin\LayerLoaderProbe.exe /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\retail_profile_probe.cpp /Fo:build\retail_profile_probe.obj /Fe:bin\RetailProfileProbe.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\vulkan_smoke.cpp /Fo:build\vulkan_smoke.obj /Fe:bin\VulkanSmoke.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\appearance_tests.cpp /Fo:build\appearance_tests.obj /Fe:bin\AppearanceTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\performance_tests.cpp /Fo:build\performance_tests.obj /Fe:bin\PerformanceTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\debug_hud_stack_tests.cpp /Fo:build\debug_hud_stack_tests.obj /Fe:bin\DebugHudStackTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\enemy_telemetry_tests.cpp /Fo:build\enemy_telemetry_tests.obj /Fe:bin\EnemyTelemetryTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\game_timer_tests.cpp /Fo:build\game_timer_tests.obj /Fe:bin\GameTimerTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\invisible_numbers_tests.cpp /Fo:build\invisible_numbers_tests.obj /Fe:bin\InvisibleNumbersTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\speedrun_tech_tests.cpp /Fo:build\speedrun_tech_tests.obj /Fe:bin\SpeedrunTechTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\health_bar_render_tests.cpp /Fo:build\health_bar_render_tests.obj /Fe:bin\HealthBarRenderTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\debug_hud_render_tests.cpp /Fo:build\debug_hud_render_tests.obj /Fe:bin\DebugHudRenderTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /W4 /O2 /EHsc /MD /I"%VULKAN_HEADERS%" tests\sync_interaction_tests.cpp /Fo:build\sync_interaction_tests.obj /Fe:bin\SyncInteractionTests.exe user32.lib /link /INCREMENTAL:NO
if errorlevel 1 exit /b %errorlevel%
if errorlevel 1 exit /b %errorlevel%
exit /b %errorlevel%
