[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ManifestPath,

    [ValidateRange(10, 180)]
    [int] $TimeoutSeconds = 60,

    [switch] $Interactive
)

$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath (Resolve-Path -LiteralPath $ManifestPath).Path -Raw | ConvertFrom-Json
& (Join-Path $PSScriptRoot 'Verify-PackagedBuild.ps1') -ManifestPath $ManifestPath
$runId = [guid]::NewGuid().ToString('N')
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$mode = if ($Interactive) { 'Interactive' } else { 'Headless' }
$processRole = "Packaged$mode"
$artifactDirectory = Join-Path $manifest.outputDirectory "RuntimeEvidence\$stamp-$($runId.Substring(0, 8))"
New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
$logPath = Join-Path $artifactDirectory 'packaged-smoke.log'
$eventPath = Join-Path $artifactDirectory 'packaged-smoke.jsonl'
$reportPath = Join-Path $artifactDirectory 'packaged-smoke-report.json'
$startedUtc = [datetime]::UtcNow

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $manifest.mainExecutable
$startInfo.UseShellExecute = $false
$launchArguments = @(
    "-AuthorityRunId=$runId", "-AuthorityProcessRole=$processRole",
    "-AuthorityEventLog=$eventPath", "-abslog=$logPath"
)
if ($Interactive) {
    $launchArguments += @('-d3d11', '-windowed', '-ResX=1280', '-ResY=720', '-nosplash', '-AuthorityExitAfter=12')
} else {
    $launchArguments += @('-nullrhi', '-unattended', '-nosplash', '-NoSound', '-AuthorityExitAfter=8')
}
foreach ($argument in $launchArguments) {
    $startInfo.ArgumentList.Add($argument)
}
$process = [System.Diagnostics.Process]::Start($startInfo)
$ownedId = $process.Id
$ownedStartTime = $process.StartTime
$ownedPath = [System.IO.Path]::GetFullPath($manifest.mainExecutable)
try {
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        throw "Packaged smoke timed out for PID $ownedId."
    }
    if ($process.ExitCode -ne 0) {
        throw "Packaged smoke exited $($process.ExitCode); see $logPath"
    }
}
finally {
    $current = Get-Process -Id $ownedId -ErrorAction SilentlyContinue
    if ($current -and $current.StartTime -eq $ownedStartTime -and
        [string]::Equals($current.MainModule.FileName, $ownedPath, [StringComparison]::OrdinalIgnoreCase)) {
        Stop-Process -Id $ownedId
        Wait-Process -Id $ownedId -Timeout 15 -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Path -LiteralPath $eventPath -PathType Leaf)) {
    throw 'Packaged smoke produced no structured event stream.'
}
$events = @(Get-Content -LiteralPath $eventPath | ForEach-Object { $_ | ConvertFrom-Json })
if ($events.Count -lt 2) {
    throw 'Packaged smoke event stream is unexpectedly short.'
}
foreach ($event in $events) {
    if ($event.schemaVersion -ne 1 -or $event.runId -ne $runId -or
        $event.processRole -ne $processRole) {
        throw 'Packaged smoke event stream identity mismatch.'
    }
}
foreach ($eventName in @('ServerReady', 'ArenaReady')) {
    if (@($events | Where-Object event -eq $eventName).Count -lt 1) {
        throw "Packaged smoke event stream is missing $eventName."
    }
}
if ((Test-Path -LiteralPath $logPath -PathType Leaf) -and
    (Select-String -LiteralPath $logPath -Pattern 'Fatal error:|Unhandled Exception:' -Quiet)) {
    throw 'Packaged smoke log contains a fatal error.'
}
$report = [ordered]@{
    schemaVersion = 1
    result = 'PASS'
    runId = $runId
    mode = $mode
    sourceSha = $manifest.sourceSha
    packageExecutableSha256 = $manifest.mainExecutableSha256
    gameExecutableSha256 = $manifest.gameExecutableSha256
    packageFingerprintSha256 = $manifest.packageFingerprintSha256
    process = [ordered]@{
        pid = $ownedId
        exitCode = $process.ExitCode
        executable = $ownedPath
    }
    observations = [ordered]@{
        eventCount = $events.Count
        serverReady = $true
        arenaReady = $true
        durationMs = [Math]::Round(([datetime]::UtcNow - $startedUtc).TotalMilliseconds, 3)
    }
    eventStream = $eventPath
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $reportPath -Encoding utf8NoBOM
Write-Output "PASS packaged-smoke mode=$mode exe=$($manifest.mainExecutable) report=$reportPath"
