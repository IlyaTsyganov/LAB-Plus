param(
    [string]$Repository = $env:GITHUB_REPOSITORY,
    [string]$SourceCommit = $env:GITHUB_SHA,
    [string]$SourceDirectory = (Join-Path $PSScriptRoot '../docs/wiki'),
    [string]$CloneDirectory = (Join-Path ([IO.Path]::GetTempPath()) ('labplus-wiki-' + [guid]::NewGuid().ToString('N')))
)

$ErrorActionPreference = 'Stop'
if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') { throw 'Expected the source repository in owner/name form.' }
if ($SourceCommit -notmatch '^[0-9a-fA-F]{40}$') { throw 'Expected the exact source commit SHA.' }
if ([string]::IsNullOrWhiteSpace($env:GH_TOKEN)) { throw 'GH_TOKEN must contain the repository job token.' }
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$pages = @(Get-ChildItem -LiteralPath $source -File -Filter '*.md' | Sort-Object Name)
if (!$pages.Count -or !($pages.Name -ccontains 'Home.md')) { throw 'The versioned Wiki source must include Home.md.' }
$clone = [IO.Path]::GetFullPath($CloneDirectory)
if (Test-Path -LiteralPath $clone) { throw 'The Wiki clone destination must not already exist.' }
$remote = "https://github.com/$Repository.wiki.git"

function Invoke-WikiGit {
    param([string[]]$Arguments, [string]$Operation, [int[]]$AllowedExitCodes = @(0))
    $ErrorActionPreference = 'Continue'
    $PSNativeCommandUseErrorActionPreference = $false
    $output = @(& git @Arguments 2>&1 | ForEach-Object { $_.ToString() })
    $code = $LASTEXITCODE
    if ($AllowedExitCodes -notcontains $code) {
        $message = $output -join [Environment]::NewLine
        if ($message -match '(?i)403|authentication failed|permission.*denied|write access.*not granted|could not read username') {
            throw "GitHub denied Wiki access during $Operation. The repository GITHUB_TOKEN may not permit Wiki Git access. No force push or page deletion was attempted.`n$message"
        }
        throw "Wiki Git operation failed during $Operation (exit $code). Existing Wiki pages were not deleted and no force push was attempted.`n$message"
    }
    [pscustomobject]@{ ExitCode = $code; Output = $output }
}

# Credentials exist only in inherited process configuration. The repository URL
# and on-disk Git config never contain the token or Authorization header.
$encoded = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes('x-access-token:' + $env:GH_TOKEN))
Write-Host "::add-mask::$encoded"
$header = "AUTHORIZATION: basic $encoded"
Write-Host "::add-mask::$header"
$configCount = 0
if ($env:GIT_CONFIG_COUNT -and ![int]::TryParse($env:GIT_CONFIG_COUNT, [ref]$configCount)) {
    throw 'Inherited Git process configuration is invalid.'
}
if ($configCount -lt 0 -or $configCount -gt 100) { throw 'Inherited Git process configuration count is invalid.' }
$keyName = "GIT_CONFIG_KEY_$configCount"
$valueName = "GIT_CONFIG_VALUE_$configCount"
$changes = @{
    GIT_CONFIG_COUNT = [string]($configCount + 1)
    GIT_TERMINAL_PROMPT = '0'
    GCM_INTERACTIVE = 'Never'
    GIT_TRACE = $null
    GIT_TRACE_CURL = $null
    GIT_CURL_VERBOSE = $null
}
$changes[$keyName] = "http.$remote.extraheader"
$changes[$valueName] = $header
$previous = @{}
foreach ($name in $changes.Keys) {
    $previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
    [Environment]::SetEnvironmentVariable($name, $changes[$name], 'Process')
}
try {
    Invoke-WikiGit -Arguments @('clone', '--', $remote, $clone) -Operation 'clone of the existing Wiki' | Out-Null
    $symbolic = Invoke-WikiGit -Arguments @('-C', $clone, 'symbolic-ref', '--quiet', '--short', 'refs/remotes/origin/HEAD') -Operation 'default branch detection'
    $tracking = ($symbolic.Output -join '').Trim()
    if (!$tracking.StartsWith('origin/') -or $tracking.Length -le 7) { throw 'The Wiki remote does not expose a default branch.' }
    $branch = $tracking.Substring(7)
    $current = Invoke-WikiGit -Arguments @('-C', $clone, 'symbolic-ref', '--quiet', '--short', 'HEAD') -Operation 'local branch verification'
    if (($current.Output -join '').Trim() -cne $branch) { throw 'The cloned Wiki is not on its remote default branch.' }

    # Every source Markdown page is managed. Other remote pages, attachments and
    # files are retained; removing a source file never deletes a remote page.
    foreach ($page in $pages) {
        Copy-Item -LiteralPath $page.FullName -Destination (Join-Path $clone $page.Name)
    }
    $paths = @($pages | ForEach-Object { $_.Name })
    Invoke-WikiGit -Arguments (@('-C', $clone, 'add', '--') + $paths) -Operation 'staging managed Markdown pages' | Out-Null
    $difference = Invoke-WikiGit -Arguments @('-C', $clone, 'diff', '--cached', '--quiet', '--exit-code') -Operation 'managed page comparison' -AllowedExitCodes @(0, 1)
    $updated = $difference.ExitCode -eq 1
    if ($updated) {
        Invoke-WikiGit -Arguments @('-C', $clone, '-c', 'user.name=github-actions[bot]', '-c', 'user.email=41898282+github-actions[bot]@users.noreply.github.com',
            'commit', '-m', "Sync versioned Wiki pages from $SourceCommit") -Operation 'Wiki commit' | Out-Null
        Invoke-WikiGit -Arguments @('-C', $clone, 'push', 'origin', "HEAD:refs/heads/$branch") -Operation 'fast-forward Wiki push' | Out-Null
    }

    $local = Invoke-WikiGit -Arguments @('-C', $clone, 'rev-parse', 'HEAD') -Operation 'local commit verification'
    $expected = ($local.Output -join '').Trim()
    if ($expected -notmatch '^[0-9a-f]{40}$') { throw 'The local Wiki commit SHA is invalid.' }
    $remoteHead = Invoke-WikiGit -Arguments @('-C', $clone, 'ls-remote', '--symref', 'origin', 'HEAD') -Operation 'remote HEAD verification'
    $actualBranch = $null
    $actualCommit = $null
    foreach ($line in $remoteHead.Output) {
        if ($line -match '^ref:\s+refs/heads/(.+)\s+HEAD$') { $actualBranch = $Matches[1] }
        if ($line -match '^([0-9a-f]{40})\s+HEAD$') { $actualCommit = $Matches[1] }
    }
    if ($actualBranch -cne $branch -or $actualCommit -cne $expected) {
        throw 'The remote Wiki HEAD changed or did not match the expected commit. No force push will be attempted.'
    }
    $result = if ($updated) { 'Updated' } else { 'Already current' }
    Write-Output "$result`: $Repository Wiki; branch $branch; commit $expected; $($pages.Count) managed Markdown pages. Remote HEAD verified."
    if ($env:GITHUB_STEP_SUMMARY) {
        "$result Wiki: https://github.com/$Repository/wiki`n`nBranch: $branch; commit: $expected; managed pages: $($pages.Count). Remote HEAD verified." |
            Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY
    }
} finally {
    foreach ($name in $previous.Keys) { [Environment]::SetEnvironmentVariable($name, $previous[$name], 'Process') }
}
