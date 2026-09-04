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
    'scripts\Run-Smoke.ps1'
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
if ($allCpp -match 'BlueprintImplementableEvent|BlueprintNativeEvent') {
    throw 'PACT-00 source may not delegate required behavior to Blueprint events.'
}

Write-Output "PASS project-structure files=$($requiredFiles.Count + 1) engine=5.8"
