[CmdletBinding()]
param(
    [ValidateSet('ConnectionMovement')]
    [string] $Scenario = 'ConnectionMovement',

    [ValidateRange(20, 180)]
    [int] $TimeoutSeconds = 75
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = (Resolve-Path -LiteralPath (Join-Path $repositoryRoot 'AuthorityArena.uproject')).Path
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')
$runId = [guid]::NewGuid().ToString('N')
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runDirectory = Join-Path $repositoryRoot "Artifacts\multiplayer\$stamp-$($runId.Substring(0, 8))"
New-Item -ItemType Directory -Path $runDirectory | Out-Null

function Get-FreeUdpPort {
    $socket = [System.Net.Sockets.UdpClient]::new(0)
    try {
        return ([System.Net.IPEndPoint]$socket.Client.LocalEndPoint).Port
    }
    finally {
        $socket.Dispose()
    }
}

function Start-OwnedProcess {
    param(
        [Parameter(Mandatory)][string] $Role,
        [Parameter(Mandatory)][string] $Executable,
        [Parameter(Mandatory)][string[]] $Arguments
    )

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
    param([Parameter(Mandatory)] $OwnedProcess)

    $current = Get-Process -Id $OwnedProcess.Id -ErrorAction SilentlyContinue
    if ($null -eq $current) {
        return
    }
    $currentPath = $current.MainModule.FileName
    if ($current.StartTime -ne $OwnedProcess.StartTime -or
        -not [string]::Equals($currentPath, $OwnedProcess.Executable, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to stop PID $($OwnedProcess.Id): process identity no longer matches owned $($OwnedProcess.Role)."
    }
    Stop-Process -Id $OwnedProcess.Id -ErrorAction Stop
    Wait-Process -Id $OwnedProcess.Id -Timeout 15 -ErrorAction SilentlyContinue
}

function Wait-LogMarkers {
    param(
        [Parameter(Mandatory)] $OwnedProcess,
        [Parameter(Mandatory)][string] $LogPath,
        [Parameter(Mandatory)][string[]] $Markers,
        [Parameter(Mandatory)][datetime] $Deadline
    )

    while ([datetime]::UtcNow -lt $Deadline) {
        if ($OwnedProcess.Process.HasExited) {
            throw "$($OwnedProcess.Role) exited $($OwnedProcess.Process.ExitCode) before markers '$($Markers -join ', ')'; see $LogPath"
        }
        if (Test-Path -LiteralPath $LogPath -PathType Leaf) {
            $text = Get-Content -LiteralPath $LogPath -Raw
            $allPresent = $true
            foreach ($marker in $Markers) {
                if (-not $text.Contains($marker, [StringComparison]::Ordinal)) {
                    $allPresent = $false
                    break
                }
            }
            if ($allPresent) {
                return
            }
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Timed out waiting for $($OwnedProcess.Role) markers '$($Markers -join ', ')'; see $LogPath"
}

function Wait-OwnedExit {
    param(
        [Parameter(Mandatory)] $OwnedProcess,
        [Parameter(Mandatory)][datetime] $Deadline
    )

    while (-not $OwnedProcess.Process.HasExited -and [datetime]::UtcNow -lt $Deadline) {
        Start-Sleep -Milliseconds 250
    }
    if (-not $OwnedProcess.Process.HasExited) {
        throw "Timed out waiting for $($OwnedProcess.Role) PID $($OwnedProcess.Id) to exit."
    }
    if ($OwnedProcess.Process.ExitCode -ne 0) {
        throw "$($OwnedProcess.Role) exited with code $($OwnedProcess.Process.ExitCode)."
    }
}

function Require-Text {
    param(
        [Parameter(Mandatory)][string] $Text,
        [Parameter(Mandatory)][string] $Needle,
        [Parameter(Mandatory)][string] $Description
    )
    if (-not $Text.Contains($Needle, [StringComparison]::Ordinal)) {
        throw "Missing assertion '$Description' (needle '$Needle')."
    }
}

$port = Get-FreeUdpPort
$deadline = [datetime]::UtcNow.AddSeconds($TimeoutSeconds)
$ownedProcesses = [System.Collections.Generic.List[object]]::new()
$serverLog = Join-Path $runDirectory 'server.log'
$client1Log = Join-Path $runDirectory 'client1.log'
$client2Log = Join-Path $runDirectory 'client2.log'
$reportPath = Join-Path $runDirectory 'report.json'

try {
    $commonArguments = @(
        '-unattended',
        '-nullrhi',
        '-nosplash',
        '-NoSound',
        '-Multiprocess',
        "-AuthorityRunId=$runId"
    )

    $server = Start-OwnedProcess -Role 'Server' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        '/Engine/Maps/Entry?listen',
        '-server',
        "-port=$port",
        '-AuthorityExitAfter=14',
        "-abslog=$serverLog"
    ) + $commonArguments)
    $ownedProcesses.Add($server)
    Wait-LogMarkers -OwnedProcess $server -LogPath $serverLog -Markers @(
        'IpNetDriver listening on port',
        'AA_EVENT event=ServerReady'
    ) -Deadline $deadline

    $client1 = Start-OwnedProcess -Role 'Client1' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        "127.0.0.1:$port`?PlayerId=Client1",
        '-game',
        '-AuthorityAutoMove',
        '-AuthorityMoveDuration=2',
        '-AuthorityExitAfter=8',
        "-abslog=$client1Log"
    ) + $commonArguments)
    $ownedProcesses.Add($client1)

    $client2 = Start-OwnedProcess -Role 'Client2' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        "127.0.0.1:$port`?PlayerId=Client2",
        '-game',
        '-AuthorityAutoMove',
        '-AuthorityMoveDuration=2',
        '-AuthorityExitAfter=8',
        "-abslog=$client2Log"
    ) + $commonArguments)
    $ownedProcesses.Add($client2)

    Wait-LogMarkers -OwnedProcess $client1 -LogPath $client1Log -Markers @(
        'local_role=AutonomousProxy',
        'local_role=SimulatedProxy',
        'AA_EVENT event=AutoMoveComplete'
    ) -Deadline $deadline
    Wait-LogMarkers -OwnedProcess $client2 -LogPath $client2Log -Markers @(
        'local_role=AutonomousProxy',
        'local_role=SimulatedProxy',
        'AA_EVENT event=AutoMoveComplete'
    ) -Deadline $deadline

    Wait-OwnedExit -OwnedProcess $client1 -Deadline $deadline
    Wait-OwnedExit -OwnedProcess $client2 -Deadline $deadline
    Wait-OwnedExit -OwnedProcess $server -Deadline $deadline

    $serverText = Get-Content -LiteralPath $serverLog -Raw
    $client1Text = Get-Content -LiteralPath $client1Log -Raw
    $client2Text = Get-Content -LiteralPath $client2Log -Raw

    foreach ($logText in @($serverText, $client1Text, $client2Text)) {
        if ($logText -match 'Fatal error:|Unhandled Exception:|CDO Constructor \(AuthorityArena') {
            throw 'A multiplayer process log contains a fatal or AuthorityArena CDO construction error.'
        }
    }

    Require-Text $serverText 'player=Client1 count=' 'server accepted Client1'
    Require-Text $serverText 'player=Client2 count=' 'server accepted Client2'
    Require-Text $serverText 'event=AuthorityPosition' 'server emitted authoritative positions'
    Require-Text $serverText 'event=AuthoritySnapshotComplete context=AuthorityArenaGameMode_0 count=2' 'server captured both players before disconnect'
    Require-Text $serverText 'player=Client1 x=' 'server position for Client1'
    Require-Text $serverText 'player=Client2 x=' 'server position for Client2'
    Require-Text $serverText 'role=Authority' 'server observed authority roles'
    Require-Text $client1Text 'event=AutoMoveComplete' 'Client1 completed movement input'
    Require-Text $client2Text 'event=AutoMoveComplete' 'Client2 completed movement input'

    $client1Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client1 x=(-?\d+(?:\.\d+)?)')
    $client2Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client2 x=(-?\d+(?:\.\d+)?)')
    if (-not $client1Position.Success -or -not $client2Position.Success) {
        throw 'Unable to parse both authoritative X positions.'
    }
    $client1X = [double]::Parse($client1Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    $client2X = [double]::Parse($client2Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    if ([Math]::Abs($client1X - (-600.0)) -lt 100.0) {
        throw "Client1 authoritative movement was too small: x=$client1X"
    }
    if ([Math]::Abs($client2X - 600.0) -lt 100.0) {
        throw "Client2 authoritative movement was too small: x=$client2X"
    }

    $sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
    $workingTreeDirty = -not [string]::IsNullOrWhiteSpace(
        ((& git -C $repositoryRoot status --porcelain) -join "`n"))
    $report = [ordered]@{
        schemaVersion = 1
        runId = $runId
        scenario = $Scenario
        sourceSha = $sourceSha
        workingTreeDirty = $workingTreeDirty
        port = $port
        result = 'PASS'
        assertions = [ordered]@{
            distinctProcessCount = @($server.Id, $client1.Id, $client2.Id | Select-Object -Unique).Count
            connectedPlayers = 2
            autonomousProxyObserved = $true
            simulatedProxyObserved = $true
            authorityObserved = $true
            client1AuthorityX = $client1X
            client2AuthorityX = $client2X
            movementOccurred = $true
        }
        processes = @(
            [ordered]@{ role = $server.Role; pid = $server.Id; executable = $server.Executable; exitCode = $server.Process.ExitCode },
            [ordered]@{ role = $client1.Role; pid = $client1.Id; executable = $client1.Executable; exitCode = $client1.Process.ExitCode },
            [ordered]@{ role = $client2.Role; pid = $client2.Id; executable = $client2.Executable; exitCode = $client2.Process.ExitCode }
        )
    }
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding utf8NoBOM
    Write-Output "PASS multiplayer scenario=$Scenario run=$runId port=$port report=$reportPath"
}
finally {
    for ($index = $ownedProcesses.Count - 1; $index -ge 0; --$index) {
        Stop-OwnedProcess -OwnedProcess $ownedProcesses[$index]
    }
}
