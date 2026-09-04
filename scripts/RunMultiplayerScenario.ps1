[CmdletBinding()]
param(
    [ValidateSet('ConnectionMovement', 'Lifecycle', 'Combat', 'DashRejected', 'AuthorityAbuse', 'AttackFlood', 'DeadAbility', 'DuplicateRespawn', 'ClientDisconnect', 'ServerShutdown', 'SecondClientConnectFail', 'Watchdog')]
    [string] $Scenario = 'ConnectionMovement',

    [ValidateRange(20, 180)]
    [int] $TimeoutSeconds = 75,

    [ValidateSet('Baseline', 'Lag60', 'Lag120', 'Jitter', 'Loss')]
    [string] $NetworkProfile = 'Baseline',

    [ValidateSet('Editor', 'Packaged')]
    [string] $Build = 'Editor',

    [string] $PackageManifest
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = (Resolve-Path -LiteralPath (Join-Path $repositoryRoot 'AuthorityArena.uproject')).Path
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')
$processExecutable = $ue.EditorCmd
$processPrefixArguments = @($projectPath)
$packageExecutableSha256 = $null
$packageSourceSha = $null
if ($Build -eq 'Packaged') {
    if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw '-PackageManifest is required when -Build Packaged.'
    }
    & (Join-Path $PSScriptRoot 'Verify-PackagedBuild.ps1') -ManifestPath $PackageManifest
    $package = Get-Content -LiteralPath (Resolve-Path -LiteralPath $PackageManifest).Path -Raw | ConvertFrom-Json
    $processExecutable = $package.mainExecutable
    $processPrefixArguments = @()
    $packageExecutableSha256 = $package.mainExecutableSha256
    $packageSourceSha = $package.sourceSha
    if ($NetworkProfile -ne 'Baseline') {
        throw 'Packaged multi-process validation currently requires NetworkProfile Baseline so applied emulation can remain directly auditable.'
    }
}
$runId = [guid]::NewGuid().ToString('N')
$runStartedUtc = [datetime]::UtcNow
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

function Get-ValidatedEventCount {
    param(
        [Parameter(Mandatory)][string] $Path,
        [Parameter(Mandatory)][string] $ExpectedRole
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing structured event stream for $ExpectedRole at $Path"
    }
    $events = @(
        Get-Content -LiteralPath $Path |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            ForEach-Object { $_ | ConvertFrom-Json }
    )
    if ($events.Count -eq 0) {
        throw "Structured event stream for $ExpectedRole is empty."
    }
    $lastSequence = 0
    foreach ($event in $events) {
        if ($event.schemaVersion -ne 1 -or $event.runId -ne $runId -or
            $event.processRole -ne $ExpectedRole -or $event.sequence -le $lastSequence) {
            throw "Invalid structured event for $ExpectedRole at sequence $($event.sequence)."
        }
        $lastSequence = $event.sequence
    }
    return $events.Count
}

function Get-StructuredEventText {
    param([Parameter(Mandatory)][string] $Path)
    return (@(
        Get-Content -LiteralPath $Path |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            ForEach-Object {
                $event = $_ | ConvertFrom-Json
                "AA_EVENT event=$($event.event) context=$($event.context) $($event.details)"
            }
    ) -join "`n")
}

function Write-ExpectedFaultReport {
    param(
        [Parameter(Mandatory)][string] $Fault,
        [Parameter(Mandatory)] $Server,
        [Parameter(Mandatory)] $Client1,
        [Parameter(Mandatory)] $Client2,
        [Parameter(Mandatory)][hashtable] $Assertions
    )

    $sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
    $workingTreeDirty = -not [string]::IsNullOrWhiteSpace(
        ((& git -C $repositoryRoot status --porcelain) -join "`n"))
    $report = [ordered]@{
        schemaVersion = 1
        runId = $runId
        scenario = $Scenario
        networkProfile = $NetworkProfile
        build = $Build
        sourceSha = $sourceSha
        workingTreeDirty = $workingTreeDirty
        result = 'PASS_EXPECTED_FAULT'
        expectedFault = $Fault
        packageExecutableSha256 = $packageExecutableSha256
        packageSourceSha = $packageSourceSha
        assertions = $Assertions
        processes = @(
            [ordered]@{ role = $Server.Role; pid = $Server.Id; exitCode = $Server.Process.ExitCode },
            [ordered]@{ role = $Client1.Role; pid = $Client1.Id; exitCode = $Client1.Process.ExitCode },
            [ordered]@{ role = $Client2.Role; pid = $Client2.Id; exitCode = $Client2.Process.ExitCode }
        )
        observations = [ordered]@{
            wallClockDurationMs = [Math]::Round(([datetime]::UtcNow - $runStartedUtc).TotalMilliseconds, 3)
            deterministicTimingClaim = $false
        }
    }
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding utf8NoBOM
    Write-Output "PASS expected-fault scenario=$Scenario run=$runId report=$reportPath"
}

$port = Get-FreeUdpPort
$deadline = [datetime]::UtcNow.AddSeconds($TimeoutSeconds)
$ownedProcesses = [System.Collections.Generic.List[object]]::new()
$serverLog = Join-Path $runDirectory 'server.log'
$client1Log = Join-Path $runDirectory 'client1.log'
$client2Log = Join-Path $runDirectory 'client2.log'
$serverEvents = Join-Path $runDirectory 'server.jsonl'
$client1Events = Join-Path $runDirectory 'client1.jsonl'
$client2Events = Join-Path $runDirectory 'client2.jsonl'
$reportPath = Join-Path $runDirectory 'report.json'

$network = switch ($NetworkProfile) {
    'Baseline' { [ordered]@{ lagMs = 0; lagVarianceMs = 0; lossPercent = 0 } }
    'Lag60' { [ordered]@{ lagMs = 60; lagVarianceMs = 0; lossPercent = 0 } }
    'Lag120' { [ordered]@{ lagMs = 120; lagVarianceMs = 0; lossPercent = 0 } }
    'Jitter' { [ordered]@{ lagMs = 90; lagVarianceMs = 30; lossPercent = 0 } }
    'Loss' { [ordered]@{ lagMs = 80; lagVarianceMs = 15; lossPercent = 2 } }
}

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
    if ($network.lagMs -gt 0) {
        $commonArguments += "-PktLag=$($network.lagMs)"
    }
    if ($network.lagVarianceMs -gt 0) {
        $commonArguments += "-PktLagVariance=$($network.lagVarianceMs)"
    }
    if ($network.lossPercent -gt 0) {
        $commonArguments += "-PktLoss=$($network.lossPercent)"
    }

    $serverScenarioArguments = @()
    $serverExitAfter = if ($Scenario -eq 'Combat') { 32 } elseif ($Scenario -eq 'ServerShutdown') { 12 } elseif ($Scenario -eq 'Watchdog') { 0 } else { 20 }
    if ($Scenario -eq 'Lifecycle' -or $Scenario -eq 'DuplicateRespawn') {
        $serverScenarioArguments += '-AuthorityLifecycle'
    }
    if ($Scenario -eq 'DashRejected') {
        $serverScenarioArguments += '-AuthorityRejectDash'
    }
    if ($Scenario -eq 'DeadAbility') {
        $serverScenarioArguments += '-AuthorityMarkDead'
    }

    $server = Start-OwnedProcess -Role 'Server' -Executable $processExecutable -Arguments ($processPrefixArguments + @(
        '/Engine/Maps/Entry?listen',
        '-server',
        '-AuthorityProcessRole=Server',
        "-AuthorityEventLog=$serverEvents",
        "-port=$port",
        "-AuthorityExitAfter=$serverExitAfter",
        "-abslog=$serverLog"
    ) + $commonArguments + $serverScenarioArguments)
    $ownedProcesses.Add($server)
    if ($Build -eq 'Packaged') {
        Wait-LogMarkers -OwnedProcess $server -LogPath $serverEvents -Markers @(
            '"event":"ServerReady"'
        ) -Deadline $deadline
    } else {
        Wait-LogMarkers -OwnedProcess $server -LogPath $serverLog -Markers @(
            'IpNetDriver listening on port',
            'AA_EVENT event=ServerReady'
        ) -Deadline $deadline
    }

    $clientExitAfter = if ($Scenario -eq 'Combat' -or $Scenario -eq 'ServerShutdown') { 16 } elseif ($Scenario -eq 'Watchdog') { 0 } else { 8 }
    $clientScenarioArguments = @()
    if ($Scenario -eq 'Combat') {
        $clientScenarioArguments += '-AuthorityCombat'
    }
    if ($Scenario -eq 'DashRejected') {
        $clientScenarioArguments += '-AuthorityDashOnly'
    }
    if ($Scenario -eq 'AuthorityAbuse') {
        $clientScenarioArguments += '-AuthorityAbuse'
    }
    if ($Scenario -eq 'AttackFlood') {
        $clientScenarioArguments += '-AuthorityFlood'
    }
    if ($Scenario -eq 'DeadAbility') {
        $clientScenarioArguments += '-AuthorityAttackOnly'
    }
    if ($Scenario -eq 'ServerShutdown' -or $Scenario -eq 'SecondClientConnectFail') {
        $clientScenarioArguments += '-AuthorityExitOnNetworkFailure'
    }
    $clientMovementArguments = @()
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
        $Scenario -eq 'AttackFlood') {
        $clientMovementArguments += '-AuthorityAutoMove'
        $clientMovementArguments += '-AuthorityMoveDuration=2'
    }
    $client1 = Start-OwnedProcess -Role 'Client1' -Executable $processExecutable -Arguments ($processPrefixArguments + @(
        "127.0.0.1:$port`?PlayerId=Client1",
        '-game',
        '-AuthorityProcessRole=Client1',
        "-AuthorityEventLog=$client1Events",
        "-AuthorityExitAfter=$clientExitAfter",
        "-abslog=$client1Log"
    ) + $commonArguments + $clientScenarioArguments + $clientMovementArguments)
    $ownedProcesses.Add($client1)

    $client2ScenarioArguments = @()
    if ($Scenario -eq 'Lifecycle' -or $Scenario -eq 'DuplicateRespawn') {
        $client2ScenarioArguments += '-AuthorityRequestRespawnAfter=4'
    }
    if ($Scenario -eq 'DuplicateRespawn') {
        $client2ScenarioArguments += '-AuthorityDuplicateRespawn'
    }
    $client2ExitAfter = if ($Scenario -eq 'ClientDisconnect') { 6 } else { $clientExitAfter }
    $client2Port = if ($Scenario -eq 'SecondClientConnectFail') { Get-FreeUdpPort } else { $port }
    $client2 = Start-OwnedProcess -Role 'Client2' -Executable $processExecutable -Arguments ($processPrefixArguments + @(
        "127.0.0.1:$client2Port`?PlayerId=Client2",
        '-game',
        '-AuthorityProcessRole=Client2',
        "-AuthorityEventLog=$client2Events",
        "-AuthorityExitAfter=$client2ExitAfter",
        "-abslog=$client2Log"
    ) + $commonArguments + $client2ScenarioArguments + $clientScenarioArguments + $clientMovementArguments)
    $ownedProcesses.Add($client2)

    if ($Scenario -eq 'SecondClientConnectFail') {
        $client1ReadySource = if ($Build -eq 'Packaged') { $client1Events } else { $client1Log }
        Wait-LogMarkers -OwnedProcess $client1 -LogPath $client1ReadySource -Markers @(
            'local_role=AutonomousProxy'
        ) -Deadline $deadline
        $client2FailureSource = if ($Build -eq 'Packaged') { $client2Events } else { $client2Log }
        $networkFailureMarker = if ($Build -eq 'Packaged') { '"event":"NetworkFailure"' } else { 'AA_EVENT event=NetworkFailure' }
        Wait-LogMarkers -OwnedProcess $client2 -LogPath $client2FailureSource -Markers @(
            $networkFailureMarker
        ) -Deadline $deadline
        Wait-OwnedExit -OwnedProcess $client2 -Deadline $deadline
        Wait-OwnedExit -OwnedProcess $client1 -Deadline $deadline
        Wait-OwnedExit -OwnedProcess $server -Deadline $deadline
        $serverText = if ($Build -eq 'Packaged') { Get-StructuredEventText $serverEvents } else { Get-Content -LiteralPath $serverLog -Raw }
        $client2Text = if ($Build -eq 'Packaged') { Get-StructuredEventText $client2Events } else { Get-Content -LiteralPath $client2Log -Raw }
        Require-Text $serverText 'player=Client1 count=1' 'server kept first client connected'
        if ($serverText.Contains('player=Client2 count=', [StringComparison]::Ordinal)) {
            throw 'Server unexpectedly accepted Client2 on the deliberately unused port.'
        }
        Require-Text $client2Text 'AA_EVENT event=NetworkFailure' 'Client2 reported connection failure'
        $eventCounts = [ordered]@{
            server = Get-ValidatedEventCount -Path $serverEvents -ExpectedRole 'Server'
            client1 = Get-ValidatedEventCount -Path $client1Events -ExpectedRole 'Client1'
            client2 = Get-ValidatedEventCount -Path $client2Events -ExpectedRole 'Client2'
        }
        Write-ExpectedFaultReport -Fault 'SecondClientConnectFail' -Server $server -Client1 $client1 -Client2 $client2 -Assertions @{
            client1Connected = $true
            client2NetworkFailure = $true
            serverAcceptedClient2 = $false
            boundedExit = $true
            structuredEventCounts = $eventCounts
        }
        return
    }

    $clientReadyMarkers = @(
        'local_role=AutonomousProxy',
        'local_role=SimulatedProxy'
    )
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
        $Scenario -eq 'AttackFlood') {
        $clientReadyMarkers += if ($Build -eq 'Packaged') { '"event":"AutoMoveComplete"' } else { 'AA_EVENT event=AutoMoveComplete' }
    }
    $client1ReadySource = if ($Build -eq 'Packaged') { $client1Events } else { $client1Log }
    $client2ReadySource = if ($Build -eq 'Packaged') { $client2Events } else { $client2Log }
    Wait-LogMarkers -OwnedProcess $client1 -LogPath $client1ReadySource -Markers $clientReadyMarkers -Deadline $deadline
    Wait-LogMarkers -OwnedProcess $client2 -LogPath $client2ReadySource -Markers $clientReadyMarkers -Deadline $deadline

    if ($Scenario -eq 'ServerShutdown') {
        Wait-OwnedExit -OwnedProcess $server -Deadline $deadline
        $client1FailureSource = if ($Build -eq 'Packaged') { $client1Events } else { $client1Log }
        $client2FailureSource = if ($Build -eq 'Packaged') { $client2Events } else { $client2Log }
        $networkFailureMarker = if ($Build -eq 'Packaged') { '"event":"NetworkFailure"' } else { 'AA_EVENT event=NetworkFailure' }
        Wait-LogMarkers -OwnedProcess $client1 -LogPath $client1FailureSource -Markers @(
            $networkFailureMarker
        ) -Deadline $deadline
        Wait-LogMarkers -OwnedProcess $client2 -LogPath $client2FailureSource -Markers @(
            $networkFailureMarker
        ) -Deadline $deadline
        Wait-OwnedExit -OwnedProcess $client1 -Deadline $deadline
        Wait-OwnedExit -OwnedProcess $client2 -Deadline $deadline
        $serverText = if ($Build -eq 'Packaged') { Get-StructuredEventText $serverEvents } else { Get-Content -LiteralPath $serverLog -Raw }
        $client1Text = if ($Build -eq 'Packaged') { Get-StructuredEventText $client1Events } else { Get-Content -LiteralPath $client1Log -Raw }
        $client2Text = if ($Build -eq 'Packaged') { Get-StructuredEventText $client2Events } else { Get-Content -LiteralPath $client2Log -Raw }
        Require-Text $serverText 'event=ServerScenarioComplete' 'server performed early controlled shutdown'
        Require-Text $client1Text 'event=NetworkFailure' 'Client1 observed server shutdown'
        Require-Text $client2Text 'event=NetworkFailure' 'Client2 observed server shutdown'
        if ($client1Text.Contains('net_mode=Standalone', [StringComparison]::Ordinal) -or
            $client2Text.Contains('net_mode=Standalone', [StringComparison]::Ordinal)) {
            throw 'A client entered standalone fallback after early server shutdown.'
        }
        $eventCounts = [ordered]@{
            server = Get-ValidatedEventCount -Path $serverEvents -ExpectedRole 'Server'
            client1 = Get-ValidatedEventCount -Path $client1Events -ExpectedRole 'Client1'
            client2 = Get-ValidatedEventCount -Path $client2Events -ExpectedRole 'Client2'
        }
        Write-ExpectedFaultReport -Fault 'ServerShutdown' -Server $server -Client1 $client1 -Client2 $client2 -Assertions @{
            bothClientsConnected = $true
            serverExitedFirst = $true
            bothClientsObservedNetworkFailure = $true
            standaloneFallback = $false
            structuredEventCounts = $eventCounts
        }
        return
    }

    Wait-OwnedExit -OwnedProcess $client1 -Deadline $deadline
    Wait-OwnedExit -OwnedProcess $client2 -Deadline $deadline
    Wait-OwnedExit -OwnedProcess $server -Deadline $deadline

    $serverText = if ($Build -eq 'Packaged') { Get-StructuredEventText $serverEvents } else { Get-Content -LiteralPath $serverLog -Raw }
    $client1Text = if ($Build -eq 'Packaged') { Get-StructuredEventText $client1Events } else { Get-Content -LiteralPath $client1Log -Raw }
    $client2Text = if ($Build -eq 'Packaged') { Get-StructuredEventText $client2Events } else { Get-Content -LiteralPath $client2Log -Raw }
    $eventCounts = [ordered]@{
        server = Get-ValidatedEventCount -Path $serverEvents -ExpectedRole 'Server'
        client1 = Get-ValidatedEventCount -Path $client1Events -ExpectedRole 'Client1'
        client2 = Get-ValidatedEventCount -Path $client2Events -ExpectedRole 'Client2'
    }

    foreach ($logText in @($serverText, $client1Text, $client2Text)) {
        if ($logText -match 'Fatal error:|Unhandled Exception:|CDO Constructor \(AuthorityArena') {
            throw 'A multiplayer process log contains a fatal or AuthorityArena CDO construction error.'
        }
    }

    if ($Build -eq 'Editor' -and $network.lagMs -gt 0) {
        foreach ($logText in @($serverText, $client1Text, $client2Text)) {
            Require-Text $logText "PktLag set to $($network.lagMs)" "$NetworkProfile lag applied"
        }
    }
    if ($Build -eq 'Editor' -and $network.lagVarianceMs -gt 0) {
        foreach ($logText in @($serverText, $client1Text, $client2Text)) {
            Require-Text $logText "PktLagVariance set to $($network.lagVarianceMs)" "$NetworkProfile variance applied"
        }
    }
    if ($Build -eq 'Editor' -and $network.lossPercent -gt 0) {
        foreach ($logText in @($serverText, $client1Text, $client2Text)) {
            Require-Text $logText "PktLoss set to $($network.lossPercent)" "$NetworkProfile loss applied"
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
    if ($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
        $Scenario -eq 'AttackFlood') {
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
    if ($Scenario -eq 'Lifecycle' -or $Scenario -eq 'DuplicateRespawn') {
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
    $duplicateRespawnRejected = $false
    if ($Scenario -eq 'DuplicateRespawn') {
        Require-Text $client2Text 'event=RespawnRequestedDuplicate' 'Client2 sent a duplicate respawn request'
        Require-Text $serverText 'event=RespawnRejected context=AuthorityArenaGameMode_0 reason=RespawnPending' 'server rejected duplicate respawn while pending'
        $duplicateRespawnRejected = $true
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
    $authorityAssertions = [ordered]@{
        forbiddenStateWrite = $false
        forgedDamage = $false
        invalidTarget = $false
        targetOutOfRange = $false
        attackRateLimited = $false
        authoritativeStateUnchanged = $false
    }
    if ($Scenario -eq 'AuthorityAbuse') {
        foreach ($reason in @('ForbiddenStateWrite', 'ForgedDamage', 'InvalidTarget', 'TargetOutOfRange', 'DuplicateSequence')) {
            Require-Text $serverText "event=AuthorityProbeRejected" "server emitted rejection for $reason"
            Require-Text $serverText "reason=$reason" "server rejected $reason"
        }
        if ($serverText -notmatch 'event=AuthorityPosition.*player=Client1 .*health=100\.00 energy=100\.00 score=0 deaths=0') {
            throw 'AuthorityAbuse changed Client1 authoritative Health, Energy, Score, or Deaths.'
        }
        $authorityAssertions.forbiddenStateWrite = $true
        $authorityAssertions.forgedDamage = $true
        $authorityAssertions.invalidTarget = $true
        $authorityAssertions.targetOutOfRange = $true
        $authorityAssertions.authoritativeStateUnchanged = $true
    }
    $deadAbilityAssertions = [ordered]@{
        deadStateReplicated = $false
        abilityRejected = $false
        projectileNotSpawned = $false
        authoritativeStateUnchanged = $false
    }
    if ($Scenario -eq 'DeadAbility') {
        Require-Text $serverText 'event=DeadGateArmed context=AuthorityArenaGameMode_0 player=Client1 health=0 tag=State.Dead' 'server established actual dead state'
        $combinedAbilityLog = "$serverText`n$client1Text"
        Require-Text $combinedAbilityLog 'event=AbilityRejected' 'dead-state attack was rejected'
        Require-Text $combinedAbilityLog 'State.Dead' 'dead-state rejection identified the blocking tag'
        if ($serverText.Contains('event=ProjectileSpawned', [StringComparison]::Ordinal)) {
            throw 'DeadAbility spawned a projectile despite State.Dead.'
        }
        if ($serverText -notmatch 'event=AuthorityPosition.*player=Client1 .*health=0\.00 .*score=0 deaths=0 .*dead=true') {
            throw 'DeadAbility did not preserve the expected authoritative dead state.'
        }
        $deadAbilityAssertions.deadStateReplicated = $true
        $deadAbilityAssertions.abilityRejected = $true
        $deadAbilityAssertions.projectileNotSpawned = $true
        $deadAbilityAssertions.authoritativeStateUnchanged = $true
    }
    if ($Scenario -eq 'AttackFlood') {
        Require-Text $serverText 'event=AuthorityProbeAccepted' 'server accepted the first legal attack probe'
        Require-Text $serverText 'reason=RateLimited' 'server rejected a too-fast follow-up attack probe'
        $acceptedCount = ([regex]::Matches($serverText, 'event=AuthorityProbeAccepted')).Count
        $rateLimitedCount = ([regex]::Matches($serverText, 'event=AuthorityProbeRejected .*reason=RateLimited')).Count
        if ($acceptedCount -ne 1 -or $rateLimitedCount -lt 1) {
            throw "Unexpected flood decisions: accepted=$acceptedCount rateLimited=$rateLimitedCount"
        }
        if ($serverText -notmatch 'event=AuthorityPosition.*player=Client1 .*health=100\.00 energy=100\.00 score=0 deaths=0') {
            throw 'AttackFlood changed Client1 authoritative Health, Energy, Score, or Deaths.'
        }
        $authorityAssertions.attackRateLimited = $true
        $authorityAssertions.authoritativeStateUnchanged = $true
    }
    $clientDisconnectObserved = $Scenario -eq 'ClientDisconnect'

    $client1Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client1 x=(-?\d+(?:\.\d+)?)')
    $client2Position = [regex]::Match($serverText, 'event=AuthorityPosition.*player=Client2 x=(-?\d+(?:\.\d+)?)')
    if (-not $client1Position.Success -or -not $client2Position.Success) {
        throw 'Unable to parse both authoritative X positions.'
    }
    $client1X = [double]::Parse($client1Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    $client2X = [double]::Parse($client2Position.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    if (($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
         $Scenario -eq 'AttackFlood') -and
        [Math]::Abs($client1X - (-600.0)) -lt 100.0) {
        throw "Client1 authoritative movement was too small: x=$client1X"
    }
    if (($Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
         $Scenario -eq 'AttackFlood') -and
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
        networkProfile = $NetworkProfile
        build = $Build
        sourceSha = $sourceSha
        workingTreeDirty = $workingTreeDirty
        port = $port
        result = 'PASS'
        packageExecutableSha256 = $packageExecutableSha256
        packageSourceSha = $packageSourceSha
        observations = [ordered]@{
            configuredLagMs = $network.lagMs
            configuredLagVarianceMs = $network.lagVarianceMs
            configuredLossPercent = $network.lossPercent
            wallClockDurationMs = [Math]::Round(([datetime]::UtcNow - $runStartedUtc).TotalMilliseconds, 3)
            deterministicTimingClaim = $false
        }
        eventStreams = [ordered]@{
            server = [ordered]@{ file = 'server.jsonl'; count = $eventCounts.server }
            client1 = [ordered]@{ file = 'client1.jsonl'; count = $eventCounts.client1 }
            client2 = [ordered]@{ file = 'client2.jsonl'; count = $eventCounts.client2 }
        }
        assertions = [ordered]@{
            distinctProcessCount = @($server.Id, $client1.Id, $client2.Id | Select-Object -Unique).Count
            connectedPlayers = 2
            autonomousProxyObserved = $true
            simulatedProxyObserved = $true
            authorityObserved = $true
            client1AuthorityX = $client1X
            client2AuthorityX = $client2X
            movementOccurred = $Scenario -eq 'ConnectionMovement' -or $Scenario -eq 'Lifecycle' -or
                $Scenario -eq 'Combat' -or $Scenario -eq 'AttackFlood'
            lifecycle = $lifecycleAssertions
            duplicateRespawnRejected = $duplicateRespawnRejected
            combat = $combatAssertions
            rejection = $rejectionAssertions
            authority = $authorityAssertions
            deadAbility = $deadAbilityAssertions
            clientDisconnectObserved = $clientDisconnectObserved
        }
        processes = @(
            [ordered]@{ role = $server.Role; pid = $server.Id; executable = $server.Executable; exitCode = $server.Process.ExitCode },
            [ordered]@{ role = $client1.Role; pid = $client1.Id; executable = $client1.Executable; exitCode = $client1.Process.ExitCode },
            [ordered]@{ role = $client2.Role; pid = $client2.Id; executable = $client2.Executable; exitCode = $client2.Process.ExitCode }
        )
    }
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding utf8NoBOM
    Write-Output "PASS multiplayer scenario=$Scenario network=$NetworkProfile run=$runId port=$port report=$reportPath"
}
finally {
    for ($index = $ownedProcesses.Count - 1; $index -ge 0; --$index) {
        Stop-OwnedProcess -OwnedProcess $ownedProcesses[$index]
    }
}
