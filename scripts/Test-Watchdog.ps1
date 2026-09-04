[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$multiplayerRoot = Join-Path $repositoryRoot 'Artifacts\multiplayer'
$before = @(
    Get-ChildItem -LiteralPath $multiplayerRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object Name
)

$output = & pwsh -NoProfile -File (Join-Path $PSScriptRoot 'RunMultiplayerScenario.ps1') `
    -Scenario Watchdog -TimeoutSeconds 20 2>&1
$runnerExitCode = $LASTEXITCODE
if ($runnerExitCode -eq 0) {
    throw 'Watchdog scenario unexpectedly exited zero.'
}
$outputText = $output -join "`n"
if (-not $outputText.Contains('Timed out waiting for', [StringComparison]::Ordinal)) {
    throw "Watchdog failed for an unexpected reason: $outputText"
}

$newRun = Get-ChildItem -LiteralPath $multiplayerRoot -Directory |
    Where-Object { $before -notcontains $_.Name } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if ($null -eq $newRun) {
    throw 'Watchdog produced no preserved run directory.'
}
foreach ($logName in @('server.log', 'client1.log', 'client2.log')) {
    if (-not (Test-Path -LiteralPath (Join-Path $newRun.FullName $logName) -PathType Leaf)) {
        throw "Watchdog did not preserve $logName."
    }
}

$serverText = Get-Content -LiteralPath (Join-Path $newRun.FullName 'server.log') -Raw
$runIdMatch = [regex]::Match($serverText, 'AuthorityRunId=([a-f0-9]{32})')
if (-not $runIdMatch.Success) {
    throw 'Unable to recover watchdog RunId from the server log.'
}
$runId = $runIdMatch.Groups[1].Value
$processes = @()
foreach ($role in @('server', 'client1', 'client2')) {
    $eventPath = Join-Path $newRun.FullName "$role.jsonl"
    if (-not (Test-Path -LiteralPath $eventPath -PathType Leaf)) {
        throw "Watchdog did not preserve $role.jsonl."
    }
    $firstEvent = Get-Content -LiteralPath $eventPath -First 1 | ConvertFrom-Json
    $expectedRole = switch ($role) {
        'server' { 'Server' }
        'client1' { 'Client1' }
        'client2' { 'Client2' }
    }
    if ($firstEvent.runId -ne $runId -or $firstEvent.processRole -ne $expectedRole -or
        [int]$firstEvent.pid -le 0) {
        throw "Watchdog $role event identity is invalid."
    }
    $processes += [ordered]@{
        role = $expectedRole
        pid = [int]$firstEvent.pid
        remaining = $false
    }
}
$ownedRemainder = @(
    Get-CimInstance Win32_Process |
        Where-Object { $_.CommandLine -like "*AuthorityRunId=$runId*" }
)
if ($ownedRemainder.Count -ne 0) {
    throw "Watchdog left $($ownedRemainder.Count) owned process(es) running for $runId."
}
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
$workingTreeDirty = -not [string]::IsNullOrWhiteSpace(
    ((& git -C $repositoryRoot status --porcelain) -join "`n"))

$result = [ordered]@{
    schemaVersion = 1
    scenario = 'Watchdog'
    runId = $runId
    sourceSha = $sourceSha
    workingTreeDirty = $workingTreeDirty
    verifiedUtc = [datetime]::UtcNow.ToString('o', [Globalization.CultureInfo]::InvariantCulture)
    runnerExitCode = $runnerExitCode
    timeoutObserved = $true
    logsPreserved = $true
    ownedProcessLeakCount = 0
    processes = $processes
    result = 'PASS_EXPECTED_FAILURE'
}
$resultPath = Join-Path $newRun.FullName 'watchdog-test.json'
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $resultPath -Encoding utf8NoBOM
Write-Output "PASS watchdog expected_exit=$runnerExitCode run=$runId report=$resultPath"
