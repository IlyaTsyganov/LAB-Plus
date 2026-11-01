param([string]$Source = (Join-Path $PSScriptRoot 'build/raw-publish'))
$ErrorActionPreference='Stop'
$destination=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'publish'))
if(!$destination.StartsWith([IO.Path]::GetFullPath($PSScriptRoot)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Package destination is outside the project.'}
$sourceRoot=[IO.Path]::GetFullPath($Source)
if($sourceRoot.Equals($destination,[StringComparison]::OrdinalIgnoreCase)){throw 'Publish to build/raw-publish before packaging.'}
$files=@('LABPlus.exe','LABPlus.dll','LABPlus.deps.json','LABPlus.runtimeconfig.json','Microsoft.Web.WebView2.Core.dll','Microsoft.Web.WebView2.WinForms.dll','Native/CollisionViewer/bin/labplus-layer.dll','Native/Lighting/labplus-lighting.dll','runtimes/win-x64/native/WebView2Loader.dll')
foreach($directory in @('Assets','licenses')){
    $files+=@(Get-ChildItem -LiteralPath (Join-Path $sourceRoot $directory) -File -Recurse | ForEach-Object {[IO.Path]::GetRelativePath($sourceRoot,$_.FullName)})
}
foreach($file in $files){if(!(Test-Path -LiteralPath (Join-Path $sourceRoot $file))){throw ('Required file missing: '+$file)}}
New-Item -ItemType Directory -Path $destination -Force | Out-Null
foreach($file in $files){
    $target=Join-Path $destination $file
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $sourceRoot $file) -Destination $target
}
New-Item -ItemType Directory -Path (Join-Path $destination 'logs') -Force | Out-Null
# A running installation owns its history. Updating binaries retains logs.
$logRoot=(Join-Path $destination 'logs')+[IO.Path]::DirectorySeparatorChar
$actual=@(Get-ChildItem -LiteralPath $destination -Recurse -File | Where-Object {!$_.FullName.StartsWith($logRoot,[StringComparison]::OrdinalIgnoreCase)})
if($actual.Count -ne $files.Count -or ($actual.Name -match '^(README|CREDITS)\.md$|\.pdb$|\.log$|^settings\.json$')){throw 'Unexpected runtime package contents.'}
Write-Output ('Ready: '+(Join-Path $destination 'LABPlus.exe')+' ('+$actual.Count+' runtime files)')
