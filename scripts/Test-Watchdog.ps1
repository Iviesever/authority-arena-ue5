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
$ownedRemainder = @(
    Get-CimInstance Win32_Process |
        Where-Object { $_.CommandLine -like "*AuthorityRunId=$runId*" }
)
if ($ownedRemainder.Count -ne 0) {
    throw "Watchdog left $($ownedRemainder.Count) owned process(es) running for $runId."
}

$result = [ordered]@{
    schemaVersion = 1
    scenario = 'Watchdog'
    runId = $runId
    runnerExitCode = $runnerExitCode
    timeoutObserved = $true
    logsPreserved = $true
    ownedProcessLeakCount = 0
    result = 'PASS_EXPECTED_FAILURE'
}
$resultPath = Join-Path $newRun.FullName 'watchdog-test.json'
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $resultPath -Encoding utf8NoBOM
Write-Output "PASS watchdog expected_exit=$runnerExitCode run=$runId report=$resultPath"
