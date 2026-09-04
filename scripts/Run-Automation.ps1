[CmdletBinding()]
param(
    [string] $TestFilter = 'AuthorityArena.',
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = (Resolve-Path -LiteralPath (Join-Path $repositoryRoot 'AuthorityArena.uproject')).Path
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $suffix = [guid]::NewGuid().ToString('N').Substring(0, 8)
    $OutputDirectory = Join-Path $repositoryRoot "Artifacts\automation\$stamp-$suffix"
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$logPath = Join-Path $OutputDirectory 'automation.log'

& $ue.EditorCmd $projectPath -unattended -nullrhi -nosplash `
    "-ExecCmds=Automation RunTests $TestFilter;Quit" `
    '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$OutputDirectory" `
    "-abslog=$logPath"
$editorExitCode = $LASTEXITCODE
if ($editorExitCode -ne 0) {
    throw "UE Automation exited $editorExitCode; see $logPath"
}

$reportPath = Join-Path $OutputDirectory 'index.json'
if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
    throw "UE Automation produced no index.json in $OutputDirectory"
}
$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
if ($report.tests.Count -eq 0) {
    throw "UE Automation found no tests matching '$TestFilter'."
}
if ($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0) {
    throw "UE Automation was not clean: succeeded=$($report.succeeded) failed=$($report.failed) notRun=$($report.notRun) inProcess=$($report.inProcess)."
}
if (Select-String -LiteralPath $logPath -Pattern 'Fatal error:|Unhandled Exception:' -Quiet) {
    throw "UE Automation log contains a fatal error; see $logPath"
}
if (Select-String -LiteralPath $logPath -SimpleMatch 'CDO Constructor (AuthorityArena' -Quiet) {
    throw "UE Automation log contains an AuthorityArena CDO construction error; see $logPath"
}

Write-Output "PASS automation filter=$TestFilter succeeded=$($report.succeeded) report=$reportPath"
