# Run only fixture executables. Probes require a game installation or a GPU.
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$nativeRoot = Split-Path -Parent $PSScriptRoot
$binaryDirectory = Join-Path $nativeRoot 'bin'
$buildScript = Get-Content -LiteralPath (Join-Path $nativeRoot 'build-viewer.cmd') -Raw

# The existing MSVC build script owns source-to-executable names. Read that
# mapping rather than maintaining another list of native suites in CI.
$buildPattern = 'tests\\(?<source>[^\s]+_tests\.cpp)\s[^\r\n]*?/Fe:bin\\(?<executable>[^\s]+\.exe)'
$builtSuites = @([regex]::Matches($buildScript, $buildPattern) | ForEach-Object {
    [pscustomobject]@{
        Source = $_.Groups['source'].Value
        Executable = $_.Groups['executable'].Value
    }
})
if ($builtSuites.Count -eq 0) { throw 'No native test sources were found in build-viewer.cmd.' }

# Every *_tests.cpp must be compiled, even when its output is a manual probe
# such as RetailPatchPortProbe.exe. A newly added source cannot be skipped.
$sources = @(Get-ChildItem -LiteralPath $PSScriptRoot -File -Filter '*_tests.cpp' |
    Select-Object -ExpandProperty Name | Sort-Object)
$compiledSources = @($builtSuites.Source | Sort-Object)
$sourceDifference = @(Compare-Object $sources $compiledSources)
if ($sourceDifference.Count -ne 0) {
    $names = $sourceDifference.InputObject -join ', '
    throw "Native test sources and build-viewer.cmd differ: $names"
}

# Only *Tests.exe are automated suites. Reject missing and stale executables
# so a successful run cannot hide a partial build or an obsolete test binary.
$expected = @($builtSuites.Executable | Where-Object { $_ -like '*Tests.exe' } | Sort-Object)
if ($expected.Count -eq 0) { throw 'No automated native fixture executables were found.' }
$actual = @(Get-ChildItem -LiteralPath $binaryDirectory -File -Filter '*Tests.exe' |
    Select-Object -ExpandProperty Name | Sort-Object)
if ($actual.Count -ne $expected.Count -or @(Compare-Object $expected $actual).Count -ne 0) {
    throw 'Native fixture binaries do not match the build script. Rebuild before running fixtures.'
}

Push-Location $projectRoot
try {
    foreach ($name in $expected) {
        Write-Host "Running $name"
        & (Join-Path $binaryDirectory $name)
        if ($LASTEXITCODE -ne 0) { throw "Native fixture failed: $name (exit $LASTEXITCODE)." }
    }
}
finally { Pop-Location }

$summary = "Native fixture executables passed: $($expected.Count)."
Write-Output $summary
if ($env:GITHUB_STEP_SUMMARY) { $summary >> $env:GITHUB_STEP_SUMMARY }
