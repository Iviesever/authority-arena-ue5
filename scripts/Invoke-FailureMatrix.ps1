[CmdletBinding()]
param(
    [ValidateRange(90, 180)]
    [int] $TimeoutSeconds = 110
)

$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot 'RunMultiplayerScenario.ps1'
foreach ($scenario in @(
    'ClientDisconnect',
    'ServerShutdown',
    'SecondClientConnectFail',
    'DashRejected',
    'AuthorityAbuse',
    'AttackFlood',
    'DeadAbility',
    'DuplicateRespawn'
)) {
    & $runner -Scenario $scenario -NetworkProfile Baseline -TimeoutSeconds $TimeoutSeconds
    if ($LASTEXITCODE -ne 0) {
        throw "Failure matrix failed at scenario $scenario."
    }
}
& (Join-Path $PSScriptRoot 'Test-Watchdog.ps1')
Write-Output 'PASS failure-matrix scenarios=9'
