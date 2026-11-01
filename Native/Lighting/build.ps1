param([string]$Compiler)
$ErrorActionPreference='Stop'
if($Compiler){
    & $Compiler -std=c++17 -O2 -static -shared (Join-Path $PSScriptRoot 'lab_lighting.cpp') ('-I'+(Join-Path $PSScriptRoot '../../third_party/Vulkan-Headers/include')) -o (Join-Path $PSScriptRoot 'labplus-lighting.dll')
}else{
    & cmd /c (Join-Path $PSScriptRoot 'build.cmd')
}
if($LASTEXITCODE){throw 'LAB Plus HUD lighting build failed.'}
