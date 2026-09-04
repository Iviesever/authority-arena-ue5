[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot

function Require-File {
    param([Parameter(Mandatory)][string] $RelativePath)
    $path = Join-Path $repositoryRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing required project file: $RelativePath"
    }
    return $path
}

$projectPath = Require-File 'AuthorityArena.uproject'
$project = Get-Content -LiteralPath $projectPath -Raw | ConvertFrom-Json
if ($project.EngineAssociation -ne '5.8') {
    throw "EngineAssociation must be 5.8, got '$($project.EngineAssociation)'."
}

$moduleNames = @($project.Modules | ForEach-Object Name)
foreach ($module in @('AuthorityArenaCore', 'AuthorityArena')) {
    if ($moduleNames -notcontains $module) {
        throw "Project is missing module $module."
    }
}

$enabledPlugins = @($project.Plugins | Where-Object Enabled | ForEach-Object Name)
if ($enabledPlugins -notcontains 'GameplayAbilities') {
    throw 'Project must enable the GameplayAbilities plugin. GameplayTags and GameplayTasks are module dependencies inside that plugin, not standalone plugins.'
}

$requiredFiles = @(
    'Config\DefaultEngine.ini',
    'Config\DefaultGame.ini',
    'Config\DefaultInput.ini',
    '.gitattributes',
    'Source\AuthorityArena.Target.cs',
    'Source\AuthorityArenaEditor.Target.cs',
    'Source\AuthorityArenaServer.Target.cs',
    'Source\AuthorityArena\AuthorityArena.Build.cs',
    'Source\AuthorityArena\Public\AuthorityArena.h',
    'Source\AuthorityArena\Private\AuthorityArena.cpp',
    'Source\AuthorityArena\Public\Game\AuthorityArenaGameMode.h',
    'Source\AuthorityArena\Private\Game\AuthorityArenaGameMode.cpp',
    'Source\AuthorityArena\Public\Game\AuthorityArenaGameInstance.h',
    'Source\AuthorityArena\Private\Game\AuthorityArenaGameInstance.cpp',
    'Source\AuthorityArena\Public\Character\AuthorityArenaCharacter.h',
    'Source\AuthorityArena\Private\Character\AuthorityArenaCharacter.cpp',
    'Source\AuthorityArena\Public\World\AuthorityArenaWorldBuilder.h',
    'Source\AuthorityArena\Private\World\AuthorityArenaWorldBuilder.cpp',
    'scripts\Find-UE58.ps1',
    'scripts\Build.ps1',
    'scripts\Run-Smoke.ps1',
    'scripts\Package-Win64.ps1',
    'scripts\Verify-PackagedBuild.ps1',
    'scripts\Run-PackagedSmoke.ps1',
    'scripts\RunMultiplayerScenario.ps1',
    'scripts\Verify-CleanSource.ps1',
    'scripts\Verify-Documentation.ps1'
)
foreach ($relativePath in $requiredFiles) {
    Require-File $relativePath | Out-Null
}

$engineConfig = Get-Content -LiteralPath (Join-Path $repositoryRoot 'Config\DefaultEngine.ini') -Raw
foreach ($requiredSetting in @(
    'GameDefaultMap=/Engine/Maps/Entry',
    'GlobalDefaultGameMode=/Script/AuthorityArena.AuthorityArenaGameMode',
    'GameInstanceClass=/Script/AuthorityArena.AuthorityArenaGameInstance'
)) {
    if (-not $engineConfig.Contains($requiredSetting, [StringComparison]::Ordinal)) {
        throw "DefaultEngine.ini is missing '$requiredSetting'."
    }
}

$buildRules = Get-Content -LiteralPath (Join-Path $repositoryRoot 'Source\AuthorityArena\AuthorityArena.Build.cs') -Raw
foreach ($dependency in @('AuthorityArenaCore', 'GameplayAbilities', 'GameplayTags', 'GameplayTasks')) {
    if (-not $buildRules.Contains("`"$dependency`"", [StringComparison]::Ordinal)) {
        throw "AuthorityArena.Build.cs is missing dependency $dependency."
    }
}

$allCpp = Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Source') -Recurse -File -Include *.h,*.cpp |
    Get-Content -Raw
$allCppText = $allCpp -join "`n"
if ($allCppText -match 'BlueprintImplementableEvent|BlueprintNativeEvent') {
    throw 'PACT-00 source may not delegate required behavior to Blueprint events.'
}

$packagedSmoke = Get-Content -LiteralPath (Join-Path $repositoryRoot 'scripts\Run-PackagedSmoke.ps1') -Raw
foreach ($requiredPackagedSmokeContract in @(
    '[switch] $Interactive',
    "'-d3d11'",
    "@('ServerReady', 'ArenaReady')",
    '$startInfo.FileName = $manifest.gameExecutable'
)) {
    if (-not $packagedSmoke.Contains($requiredPackagedSmokeContract, [StringComparison]::Ordinal)) {
        throw "Packaged smoke runner is missing contract '$requiredPackagedSmokeContract'."
    }
}

$multiplayerRunner = Get-Content -LiteralPath (Join-Path $repositoryRoot 'scripts\RunMultiplayerScenario.ps1') -Raw
foreach ($requiredPackagedMultiplayerContract in @(
    "[ValidateSet('Editor', 'Packaged')]",
    '-PackageManifest is required when -Build Packaged.',
    'Get-StructuredEventText',
    '-AuthoritySuppressHostPawn'
)) {
    if (-not $multiplayerRunner.Contains($requiredPackagedMultiplayerContract, [StringComparison]::Ordinal)) {
        throw "Multiplayer runner is missing packaged-build contract '$requiredPackagedMultiplayerContract'."
    }
}

$gameModeSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'Source\AuthorityArena\Private\Game\AuthorityArenaGameMode.cpp') -Raw
if (-not $gameModeSource.Contains('AuthoritySuppressHostPawn', [StringComparison]::Ordinal)) {
    throw 'GameMode is missing explicit packaged listen-server host-pawn suppression for E2E.'
}
foreach ($finalStateContract in @('FinalAuthorityState', 'ClientFinalState')) {
    if (-not $allCppText.Contains($finalStateContract, [StringComparison]::Ordinal)) {
        throw "C++ multi-process evidence is missing '$finalStateContract'."
    }
}
foreach ($runnerEvidenceContract in @('finalConsistency', 'AttackFloodAbilityRequest')) {
    if (-not $multiplayerRunner.Contains($runnerEvidenceContract, [StringComparison]::Ordinal)) {
        throw "Multiplayer runner is missing evidence contract '$runnerEvidenceContract'."
    }
}
if (-not $allCppText.Contains('CallServerTryActivateAbility', [StringComparison]::Ordinal)) {
    throw 'Attack flood automation does not use the native GAS server activation path.'
}

$packageScript = Get-Content -LiteralPath (Join-Path $repositoryRoot 'scripts\Package-Win64.ps1') -Raw
$packageVerifier = Get-Content -LiteralPath (Join-Path $repositoryRoot 'scripts\Verify-PackagedBuild.ps1') -Raw
foreach ($requiredPackageEvidence in @(
    'gameExecutableSha256',
    'packageFingerprintSha256',
    'payloadFiles'
)) {
    if (-not $packageScript.Contains($requiredPackageEvidence, [StringComparison]::Ordinal) -or
        -not $packageVerifier.Contains($requiredPackageEvidence, [StringComparison]::Ordinal)) {
        throw "Package evidence is missing '$requiredPackageEvidence' generation or verification."
    }
}
if (-not $packageScript.Contains('ConflictingInstance', [StringComparison]::Ordinal)) {
    throw 'Package runner is missing bounded UBT mutex teardown retry handling.'
}

Write-Output "PASS project-structure files=$($requiredFiles.Count + 1) engine=5.8"
