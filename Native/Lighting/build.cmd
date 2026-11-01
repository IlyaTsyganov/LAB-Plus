@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>&1
if not errorlevel 1 goto ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS exit /b 1
call "%VCVARS%" >nul
if errorlevel 1 exit /b 1
:ready
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /EHsc /MT /LD /I"..\..\third_party\Vulkan-Headers\include" lab_lighting.cpp /Fo:build\lighting.obj /Fe:labplus-lighting.dll /link /IMPLIB:build\lighting.lib
exit /b %errorlevel%
