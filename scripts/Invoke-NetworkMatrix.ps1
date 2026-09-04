[CmdletBinding()]
param(
    [ValidateRange(90, 180)]
    [int] $TimeoutSeconds = 140
)

$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot 'RunMultiplayerScenario.ps1'
foreach ($profile in @('Baseline', 'Lag60', 'Lag120', 'Jitter', 'Loss')) {
    & $runner -Scenario Combat -NetworkProfile $profile -TimeoutSeconds $TimeoutSeconds
    if ($LASTEXITCODE -ne 0) {
        throw "Network matrix failed at profile $profile."
    }
}
Write-Output 'PASS network-matrix profiles=Baseline,Lag60,Lag120,Jitter,Loss'
