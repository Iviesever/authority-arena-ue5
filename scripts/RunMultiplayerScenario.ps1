[CmdletBinding()]
param(
    [ValidateSet('ConnectionMovement', 'Lifecycle', 'Combat', 'DashRejected')]
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
        if (Test-Path -LiteralPath $LogPath -PathType Leaf) {
            $text = $null
            $stream = $null
            $reader = $null
            try {
                $stream = [System.IO.FileStream]::new(
                    $LogPath,
                    [System.IO.FileMode]::Open,
                    [System.IO.FileAccess]::Read,
                    [System.IO.FileShare]::ReadWrite -bor [System.IO.FileShare]::Delete)
                $reader = [System.IO.StreamReader]::new($stream)
                $text = $reader.ReadToEnd()
            }
            catch [System.IO.IOException] {
                $text = $null
            }
            finally {
                if ($null -ne $reader) {
                    $reader.Dispose()
                }
                elseif ($null -ne $stream) {
                    $stream.Dispose()
                }
            }
            if (-not [string]::IsNullOrEmpty($text)) {
                $allPresent = $true
                foreach ($marker in $Markers) {
                    if ($text.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
                        $allPresent = $false
                        break
                    }
                }
                if ($allPresent) {
                    return
                }
            }
        }
        if ($OwnedProcess.Process.HasExited) {
            throw "$($OwnedProcess.Role) exited $($OwnedProcess.Process.ExitCode) before markers '$($Markers -join ', ')'; see $LogPath"
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
        '-ExecCmds=t.MaxFPS 60',
        "-AuthorityRunId=$runId"
    )

    $serverScenarioArguments = @()
    $serverExitAfter = if ($Scenario -eq 'Combat') { 32 } else { 20 }
    if ($Scenario -eq 'Lifecycle') {
        $serverScenarioArguments += '-AuthorityLifecycle'
    }
    if ($Scenario -eq 'DashRejected') {
        $serverScenarioArguments += '-AuthorityRejectDash'
    }

    $server = Start-OwnedProcess -Role 'Server' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        '/Engine/Maps/Entry?listen',
        '-server',
        "-port=$port",
        "-AuthorityExitAfter=$serverExitAfter",
        "-abslog=$serverLog"
    ) + $commonArguments + $serverScenarioArguments)
    $ownedProcesses.Add($server)
    Wait-LogMarkers -OwnedProcess $server -LogPath $serverLog -Markers @(
        'IpNetDriver listening on port',
        'AA_EVENT event=ServerReady'
    ) -Deadline $deadline

    $clientExitAfter = if ($Scenario -eq 'Combat') { 16 } else { 8 }
    $clientScenarioArguments = @()
    if ($Scenario -eq 'Combat') {
        $clientScenarioArguments += '-AuthorityCombat'
    }
    if ($Scenario -eq 'DashRejected') {
        $clientScenarioArguments += '-AuthorityDashOnly'
    }
    $clientMovementArguments = @()
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle') {
        $clientMovementArguments += '-AuthorityAutoMove'
        $clientMovementArguments += '-AuthorityMoveDuration=2'
    }
    $client1 = Start-OwnedProcess -Role 'Client1' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        "127.0.0.1:$port`?PlayerId=Client1",
        '-game',
        "-AuthorityExitAfter=$clientExitAfter",
        "-abslog=$client1Log"
    ) + $commonArguments + $clientScenarioArguments + $clientMovementArguments)
    $ownedProcesses.Add($client1)

    $client2ScenarioArguments = @()
    if ($Scenario -eq 'Lifecycle') {
        $client2ScenarioArguments += '-AuthorityRequestRespawnAfter=4'
    }
    $client2 = Start-OwnedProcess -Role 'Client2' -Executable $ue.EditorCmd -Arguments (@(
        $projectPath,
        "127.0.0.1:$port`?PlayerId=Client2",
        '-game',
        "-AuthorityExitAfter=$clientExitAfter",
        "-abslog=$client2Log"
    ) + $commonArguments + $client2ScenarioArguments + $clientScenarioArguments + $clientMovementArguments)
    $ownedProcesses.Add($client2)

    $clientReadyMarkers = @(
        'local_role=AutonomousProxy',
        'local_role=SimulatedProxy'
    )
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle') {
        $clientReadyMarkers += 'AA_EVENT event=AutoMoveComplete'
    }
    Wait-LogMarkers -OwnedProcess $client1 -LogPath $client1Log -Markers $clientReadyMarkers -Deadline $deadline
    Wait-LogMarkers -OwnedProcess $client2 -LogPath $client2Log -Markers $clientReadyMarkers -Deadline $deadline

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
    Require-Text $serverText 'event=PlayerDisconnected context=AuthorityArenaGameMode_0 player=Client1' 'Client1 disconnected cleanly'
    Require-Text $serverText 'event=PlayerDisconnected context=AuthorityArenaGameMode_0 player=Client2' 'Client2 disconnected cleanly'
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle') {
        Require-Text $client1Text 'event=AutoMoveComplete' 'Client1 completed movement input'
        Require-Text $client2Text 'event=AutoMoveComplete' 'Client2 completed movement input'
    }
    if ($client1Text.Contains('net_mode=Standalone', [StringComparison]::Ordinal) -or
        $client2Text.Contains('net_mode=Standalone', [StringComparison]::Ordinal)) {
        throw 'A client fell back into a standalone match after the authoritative server exited.'
    }
    $lifecycleAssertions = [ordered]@{
        pawnDestroyed = $false
        pawnRespawned = $false
        disconnectedCleanly = $true
    }
    if ($Scenario -eq 'Lifecycle') {
        Require-Text $serverText 'event=PawnDestroyed context=AuthorityArenaGameMode_0 player=Client2' 'Client2 pawn destroyed by authority'
        Require-Text $serverText 'event=PawnRespawned context=AuthorityArenaGameMode_0 player=Client2' 'Client2 pawn respawned by authority'
        $destroyCount = ([regex]::Matches($serverText, 'event=PawnDestroyed .*player=Client2')).Count
        $respawnCount = ([regex]::Matches($serverText, 'event=PawnRespawned .*player=Client2')).Count
        if ($destroyCount -ne 1 -or $respawnCount -ne 1) {
            throw "Lifecycle must destroy and respawn exactly once; destroyed=$destroyCount respawned=$respawnCount"
        }
        $lifecycleAssertions.pawnDestroyed = $true
        $lifecycleAssertions.pawnRespawned = $true
    }
    $combatAssertions = [ordered]@{
        dashPredicted = $false
        dashConfirmed = $false
        projectileDamage = $false
        shieldReducedDamage = $false
        death = $false
        respawn = $false
        score = $false
    }
    if ($Scenario -eq 'Combat') {
        Require-Text $client1Text 'event=DashPredicted' 'Client1 predicted Dash'
        Require-Text $serverText 'event=DashConfirmed' 'server confirmed Dash'
        Require-Text $serverText 'event=ProjectileSpawned' 'server spawned projectile'
        Require-Text $client2Text 'event=ShieldPredicted' 'Client2 predicted Shield'
        Require-Text $serverText 'event=ShieldConfirmed' 'server confirmed Shield'
        Require-Text $serverText 'event=DamageApplied' 'server applied projectile damage'
        Require-Text $serverText 'raw=34.00 applied=17.00 shield=true' 'shield halved projectile damage'
        Require-Text $serverText 'event=Death context=AuthorityArenaGameMode_0 victim=Client2 instigator=Client1' 'server recorded Client2 death'
        Require-Text $serverText 'event=Score' 'server updated score'
        Require-Text $serverText 'player=Client1 score=1' 'Client1 received one point'
        Require-Text $serverText 'event=Deaths' 'server updated death count'
        Require-Text $serverText 'player=Client2 deaths=1' 'Client2 received one death'
        Require-Text $serverText 'event=PawnRespawned context=AuthorityArenaGameMode_0 player=Client2' 'server respawned Client2 after death'
        $combatAssertions.dashPredicted = $true
        $combatAssertions.dashConfirmed = $true
        $combatAssertions.projectileDamage = $true
        $combatAssertions.shieldReducedDamage = $true
        $combatAssertions.death = $true
        $combatAssertions.respawn = $true
        $combatAssertions.score = $true
    }
    $rejectionAssertions = [ordered]@{
        clientPredicted = $false
        serverRejected = $false
        clientCorrected = $false
    }
    if ($Scenario -eq 'DashRejected') {
        Require-Text $client1Text 'event=DashPredicted' 'Client1 predicted the dash before server response'
        Require-Text $serverText 'event=DashRejectionArmed context=AuthorityArenaGameMode_0 player=Client1 reason=Failure.Resource' 'server armed a private rejection gate'
        Require-Text $serverText 'event=AbilityRejected' 'server rejected the predicted dash'
        Require-Text $serverText 'reasons=Failure.Resource' 'server used the expected rejection reason'
        $clientFinal = [regex]::Match(
            $client1Text,
            'event=ClientScenarioComplete.*player=Client1 x=(-?\d+(?:\.\d+)?).*energy=(-?\d+(?:\.\d+)?)')
        if (-not $clientFinal.Success) {
            throw 'Unable to parse Client1 final corrected position and energy.'
        }
        $clientFinalX = [double]::Parse($clientFinal.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
        $clientFinalEnergy = [double]::Parse($clientFinal.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture)
        if ([Math]::Abs($clientFinalX - (-600.0)) -gt 30.0 -or
            [Math]::Abs($clientFinalEnergy) -gt 0.01) {
            throw "Predicted Dash did not correct to authority: x=$clientFinalX energy=$clientFinalEnergy"
        }
        $rejectionAssertions.clientPredicted = $true
        $rejectionAssertions.serverRejected = $true
        $rejectionAssertions.clientCorrected = $true
    }

    $client1Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client1 x=(-?\d+(?:\.\d+)?)')
    $client2Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client2 x=(-?\d+(?:\.\d+)?)')
    if (-not $client1Position.Success -or -not $client2Position.Success) {
        throw 'Unable to parse both authoritative X positions.'
    }
    $client1X = [double]::Parse($client1Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    $client2X = [double]::Parse($client2Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    if ($Scenario -ne 'DashRejected' -and [Math]::Abs($client1X - (-600.0)) -lt 100.0) {
        throw "Client1 authoritative movement was too small: x=$client1X"
    }
    if (($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle') -and
        [Math]::Abs($client2X - 600.0) -lt 100.0) {
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
            movementOccurred = $Scenario -ne 'DashRejected'
            lifecycle = $lifecycleAssertions
            combat = $combatAssertions
            rejection = $rejectionAssertions
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
