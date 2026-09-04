[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Path,

    [switch] $RequireClean,

    [string] $ExpectedSourceSha
)

$ErrorActionPreference = 'Stop'
$resolvedPath = (Resolve-Path -LiteralPath $Path).Path
$report = Get-Content -LiteralPath $resolvedPath -Raw | ConvertFrom-Json

if ($report.schemaVersion -ne 1) {
    throw "Unsupported report schema '$($report.schemaVersion)'."
}
if ($report.result -notin @('PASS', 'PASS_EXPECTED_FAULT')) {
    throw "Scenario did not pass: result=$($report.result)"
}
if ([string]::IsNullOrWhiteSpace($report.runId) -or
    [string]::IsNullOrWhiteSpace($report.sourceSha)) {
    throw 'Report is missing runId or sourceSha.'
}
if ($RequireClean -and $report.workingTreeDirty -ne $false) {
    throw 'Report was produced from a dirty working tree.'
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceSha) -and
    $report.sourceSha -ne $ExpectedSourceSha) {
    throw "Source SHA mismatch: expected=$ExpectedSourceSha actual=$($report.sourceSha)"
}

$processes = @($report.processes)
if ($processes.Count -ne 3) {
    throw "Expected exactly three process results, got $($processes.Count)."
}
foreach ($process in $processes) {
    if ($process.exitCode -ne 0) {
        throw "Process $($process.role) exited $($process.exitCode)."
    }
}

if ($report.result -eq 'PASS') {
    foreach ($role in @('server', 'client1', 'client2')) {
        $stream = $report.eventStreams.$role
        if ($null -eq $stream -or $stream.count -lt 1 -or
            [string]::IsNullOrWhiteSpace($stream.file)) {
            throw "Report is missing a non-empty structured event stream for $role."
        }
    }
}

Write-Output "PASS scenario-report scenario=$($report.scenario) result=$($report.result) sha=$($report.sourceSha)"
