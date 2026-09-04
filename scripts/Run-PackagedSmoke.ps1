[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ManifestPath,

    [ValidateRange(10, 180)]
    [int] $TimeoutSeconds = 60
)

$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath (Resolve-Path -LiteralPath $ManifestPath).Path -Raw | ConvertFrom-Json
& (Join-Path $PSScriptRoot 'Verify-PackagedBuild.ps1') -ManifestPath $ManifestPath
$artifactDirectory = Join-Path $manifest.outputDirectory 'RuntimeEvidence'
New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null
$logPath = Join-Path $artifactDirectory 'packaged-smoke.log'

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $manifest.mainExecutable
$startInfo.UseShellExecute = $false
foreach ($argument in @(
    '-nullrhi', '-unattended', '-nosplash', '-NoSound', '-ExecCmds=quit', "-abslog=$logPath"
)) {
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

if (-not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
    throw 'Packaged smoke produced no log.'
}
foreach ($marker in @("Game class is 'AuthorityArenaGameMode'", 'AA_EVENT ArenaReady blocks=6')) {
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch $marker -Quiet)) {
        throw "Packaged smoke log is missing '$marker'."
    }
}
if (Select-String -LiteralPath $logPath -Pattern 'Fatal error:|Unhandled Exception:' -Quiet) {
    throw 'Packaged smoke log contains a fatal error.'
}
Write-Output "PASS packaged-smoke exe=$($manifest.mainExecutable) log=$logPath"
