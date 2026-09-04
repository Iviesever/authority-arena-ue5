[CmdletBinding()]
param(
    [ValidateRange(10, 300)]
    [int] $TimeoutSeconds = 90,

    [switch] $Interactive
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repositoryRoot 'AuthorityArena.uproject'
$artifactDirectory = Join-Path $repositoryRoot 'Artifacts\pact00'
$mode = if ($Interactive) { 'interactive' } else { 'headless' }
$logPath = Join-Path $artifactDirectory "$mode.log"
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')
New-Item -ItemType Directory -Path $artifactDirectory -Force | Out-Null

$resolvedArtifactDirectory = [System.IO.Path]::GetFullPath($artifactDirectory).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
$resolvedLogPath = [System.IO.Path]::GetFullPath($logPath)
if (-not $resolvedLogPath.StartsWith(
        "$resolvedArtifactDirectory$([System.IO.Path]::DirectorySeparatorChar)",
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to replace a smoke log outside $resolvedArtifactDirectory"
}
if (Test-Path -LiteralPath $resolvedLogPath -PathType Leaf) {
    Remove-Item -LiteralPath $resolvedLogPath -Force
}

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = if ($Interactive) { $ue.Editor } else { $ue.EditorCmd }
$startInfo.UseShellExecute = $false
$arguments = [System.Collections.Generic.List[string]]@(
    $projectPath,
    '/Engine/Maps/Entry',
    '-game',
    '-nosplash',
    "-abslog=$logPath"
)
if ($Interactive) {
    # UE 5.8 deny-lists the installed NVIDIA 551.61 driver for D3D12 and can
    # block on its driver dialog. D3D11 is an explicit, supported smoke path;
    # the default project RHI remains D3D12 until packaged validation.
    $arguments.Add('-d3d11')
    $arguments.Add('-windowed')
    $arguments.Add('-ResX=960')
    $arguments.Add('-ResY=540')
    $arguments.Add('-NoSound')
} else {
    $arguments.Add('-nullrhi')
    $arguments.Add('-unattended')
    $arguments.Add('-ExecCmds=quit')
}
foreach ($argument in $arguments) {
    $startInfo.ArgumentList.Add($argument)
}

$process = [System.Diagnostics.Process]::Start($startInfo)
$ownedId = $process.Id
$ownedStartTime = $process.StartTime
$ownedPath = $process.MainModule.FileName

try {
    if ($Interactive) {
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $ready = $false
        while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
            if ($process.HasExited) {
                throw "Interactive smoke exited $($process.ExitCode) before Ready; see $logPath"
            }
            if ((Test-Path -LiteralPath $logPath -PathType Leaf) -and
                (Select-String -LiteralPath $logPath -Pattern "LogLoad: Game class is 'AuthorityArenaGameMode'" -Quiet) -and
                (Select-String -LiteralPath $logPath -Pattern 'Engine is initialized. Leaving FEngineLoop::Init' -Quiet)) {
                $ready = $true
                break
            }
            Start-Sleep -Milliseconds 250
        }
        if (-not $ready) {
            throw "Interactive smoke timed out waiting for Ready after $TimeoutSeconds seconds (PID $ownedId)."
        }
    } else {
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            throw "Headless smoke timed out after $TimeoutSeconds seconds (PID $ownedId)."
        }
        if ($process.ExitCode -ne 0) {
            throw "Headless smoke process exited $($process.ExitCode); see $logPath"
        }
    }
}
finally {
    $stillRunning = Get-Process -Id $ownedId -ErrorAction SilentlyContinue
    if ($stillRunning -and
        $stillRunning.StartTime -eq $ownedStartTime -and
        $stillRunning.MainModule.FileName -eq $ownedPath) {
        Stop-Process -Id $ownedId -ErrorAction Stop
        Wait-Process -Id $ownedId -Timeout 15 -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
    throw "Smoke process produced no log at $logPath"
}
foreach ($requiredMarker in @(
    "LogLoad: Game class is 'AuthorityArenaGameMode'",
    'AA_EVENT ArenaReady blocks=6'
)) {
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch $requiredMarker -Quiet)) {
        throw "Smoke log is missing required marker '$requiredMarker'; see $logPath"
    }
}
if (Select-String -LiteralPath $logPath -Pattern 'Fatal error:|Unhandled Exception:|Engine exit requested' -Quiet) {
    $fatal = Select-String -LiteralPath $logPath -Pattern 'Fatal error:|Unhandled Exception:' -Quiet
    if ($fatal) {
        throw "Smoke log contains a fatal error; see $logPath"
    }
}

Write-Output "PASS $mode-smoke pid=$ownedId log=$logPath"
