[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$scriptsRoot = Split-Path -Parent $PSScriptRoot
$runner = Join-Path $scriptsRoot 'RunMultiplayerScenario.ps1'

$invalidOutput = & pwsh -NoProfile -File $runner `
    -Scenario ConnectionMovement -NetworkProfile InvalidProfile 2>&1
if ($LASTEXITCODE -eq 0) {
    throw 'Invalid network profile unexpectedly succeeded.'
}
if (-not (($invalidOutput -join "`n").Contains('ValidateSet', [StringComparison]::Ordinal))) {
    throw 'Invalid profile did not fail at the parameter boundary.'
}

& (Join-Path $scriptsRoot 'Test-Watchdog.ps1')
Write-Output 'PASS runner-contract invalid-profile,watchdog-cleanup'
