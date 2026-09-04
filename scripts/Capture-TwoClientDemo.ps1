[CmdletBinding()]
param(
    [string] $OutputPath = 'docs\images\authority-arena-two-clients.png',

    [ValidateRange(30, 180)]
    [int] $TimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = (Resolve-Path -LiteralPath (Join-Path $repositoryRoot 'AuthorityArena.uproject')).Path
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')
$runId = [guid]::NewGuid().ToString('N')
$portSocket = [System.Net.Sockets.UdpClient]::new(0)
try {
    $port = ([System.Net.IPEndPoint]$portSocket.Client.LocalEndPoint).Port
}
finally {
    $portSocket.Dispose()
}
$runDirectory = Join-Path $repositoryRoot "Artifacts\visual\$($runId.Substring(0, 8))"
New-Item -ItemType Directory -Path $runDirectory | Out-Null

function Start-OwnedProcess {
    param([string] $Role, [string] $Executable, [string[]] $Arguments)
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument)
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    return [pscustomobject]@{
        Role = $Role
        Process = $process
        Id = $process.Id
        StartTime = $process.StartTime
        Executable = [System.IO.Path]::GetFullPath($Executable)
    }
}

function Stop-OwnedProcess {
    param($OwnedProcess)
    $current = Get-Process -Id $OwnedProcess.Id -ErrorAction SilentlyContinue
    if ($null -eq $current) {
        return
    }
    if ($current.StartTime -ne $OwnedProcess.StartTime -or
        -not [string]::Equals($current.MainModule.FileName, $OwnedProcess.Executable, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to stop mismatched PID $($OwnedProcess.Id)."
    }
    Stop-Process -Id $OwnedProcess.Id -ErrorAction Stop
    Wait-Process -Id $OwnedProcess.Id -Timeout 15 -ErrorAction SilentlyContinue
}

function Wait-LogMarker {
    param($OwnedProcess, [string] $LogPath, [string] $Marker, [datetime] $Deadline)
    while ([datetime]::UtcNow -lt $Deadline) {
        if (Test-Path -LiteralPath $LogPath -PathType Leaf) {
            if (Select-String -LiteralPath $LogPath -SimpleMatch $Marker -Quiet) {
                return
            }
        }
        if ($OwnedProcess.Process.HasExited) {
            throw "$($OwnedProcess.Role) exited before '$Marker'; see $LogPath"
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Timed out waiting for $($OwnedProcess.Role) marker '$Marker'."
}

function Wait-MainWindow {
    param($OwnedProcess, [datetime] $Deadline)
    while ([datetime]::UtcNow -lt $Deadline) {
        $OwnedProcess.Process.Refresh()
        if ($OwnedProcess.Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $OwnedProcess.Process.MainWindowHandle
        }
        if ($OwnedProcess.Process.HasExited) {
            throw "$($OwnedProcess.Role) exited before creating a visible window."
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Timed out waiting for $($OwnedProcess.Role) window."
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AuthorityArenaWindowApi {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetWindowPos(
        IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
}
'@

$owned = [System.Collections.Generic.List[object]]::new()
$serverLog = Join-Path $runDirectory 'server.log'
$client1Log = Join-Path $runDirectory 'client1.log'
$client2Log = Join-Path $runDirectory 'client2.log'
$deadline = [datetime]::UtcNow.AddSeconds($TimeoutSeconds)
$captureSucceeded = $false

try {
    $common = @(
        '-nosplash', '-NoSound', '-Multiprocess', '-ExecCmds=t.MaxFPS 60',
        '-PktLag=60', "-AuthorityRunId=$runId"
    )
    $server = Start-OwnedProcess 'Server' $ue.EditorCmd (@(
        $projectPath, '/Engine/Maps/Entry?listen', '-server', '-unattended', '-nullrhi',
        "-port=$port", '-AuthorityExitAfter=50', '-AuthorityProcessRole=Server',
        "-AuthorityEventLog=$(Join-Path $runDirectory 'server.jsonl')", "-abslog=$serverLog"
    ) + $common)
    $owned.Add($server)
    Wait-LogMarker $server $serverLog 'IpNetDriver listening on port' $deadline

    $client1 = Start-OwnedProcess 'Client1' $ue.Editor (@(
        $projectPath, "127.0.0.1:$port`?PlayerId=Client1", '-game', '-d3d11', '-windowed',
        '-ResX=840', '-ResY=560', '-WinX=0', '-WinY=0', '-AuthorityCombat',
        '-AuthorityProcessRole=Client1', "-AuthorityEventLog=$(Join-Path $runDirectory 'client1.jsonl')",
        "-abslog=$client1Log"
    ) + $common)
    $owned.Add($client1)

    $client2 = Start-OwnedProcess 'Client2' $ue.Editor (@(
        $projectPath, "127.0.0.1:$port`?PlayerId=Client2", '-game', '-d3d11', '-windowed',
        '-ResX=840', '-ResY=560', '-WinX=850', '-WinY=0', '-AuthorityCombat',
        '-AuthorityProcessRole=Client2', "-AuthorityEventLog=$(Join-Path $runDirectory 'client2.jsonl')",
        "-abslog=$client2Log"
    ) + $common)
    $owned.Add($client2)

    Wait-LogMarker $client1 $client1Log 'local_role=AutonomousProxy' $deadline
    Wait-LogMarker $client2 $client2Log 'local_role=AutonomousProxy' $deadline
    Wait-LogMarker $server $serverLog 'event=PawnRespawned context=AuthorityArenaGameMode_0 player=Client2' $deadline

    $client1Window = Wait-MainWindow $client1 $deadline
    $client2Window = Wait-MainWindow $client2 $deadline
    $screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $halfWidth = [Math]::Floor($screen.Width / 2) - 5
    $captureHeight = [Math]::Min(620, $screen.Height)
    [AuthorityArenaWindowApi]::SetWindowPos($client1Window, [IntPtr]::Zero, 0, 0, $halfWidth, $captureHeight, 0x0040) | Out-Null
    [AuthorityArenaWindowApi]::SetWindowPos($client2Window, [IntPtr]::Zero, $halfWidth + 10, 0, $halfWidth, $captureHeight, 0x0040) | Out-Null
    Start-Sleep -Milliseconds 1500

    $resolvedOutput = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputPath))
    $allowedDirectory = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'docs\images')).TrimEnd('\') + '\'
    if (-not $resolvedOutput.StartsWith($allowedDirectory, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Screenshot output must stay inside $allowedDirectory"
    }
    if (Test-Path -LiteralPath $resolvedOutput -PathType Leaf) {
        [System.IO.File]::Delete($resolvedOutput)
    }
    $bitmap = [System.Drawing.Bitmap]::new($screen.Width, $captureHeight)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($screen.Left, $screen.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($resolvedOutput, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    if ((Get-Item -LiteralPath $resolvedOutput).Length -lt 50000) {
        throw 'Captured screenshot is unexpectedly small.'
    }
    $captureSucceeded = $true
}
finally {
    for ($index = $owned.Count - 1; $index -ge 0; --$index) {
        Stop-OwnedProcess $owned[$index]
    }
}

if (-not $captureSucceeded) {
    throw 'Two-client capture did not complete.'
}
$remaining = @($owned | Where-Object { Get-Process -Id $_.Id -ErrorAction SilentlyContinue })
if ($remaining.Count -ne 0) {
    throw "Visual capture left $($remaining.Count) owned process(es)."
}
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
$metadataPath = [System.IO.Path]::ChangeExtension(
    [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputPath)), '.json')
[ordered]@{
    schemaVersion = 1
    sourceSha = $sourceSha
    runId = $runId
    networkProfile = 'Lag60'
    serverPlusClients = 3
    combatRespawnObserved = $true
    ownedProcessLeakCount = 0
    captureWidth = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Width
    captureHeight = [Math]::Min(620, [System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Height)
} | ConvertTo-Json | Set-Content -LiteralPath $metadataPath -Encoding utf8NoBOM
Write-Output "PASS visual-capture image=$([System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputPath))) metadata=$metadataPath"
