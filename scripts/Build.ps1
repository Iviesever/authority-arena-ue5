[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Game', 'Server')]
    [string] $Target = 'Editor',

    [ValidateSet('Development', 'Shipping')]
    [string] $Configuration = 'Development',

    [switch] $SkipCoreTests
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repositoryRoot 'AuthorityArena.uproject'
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')

if (-not $SkipCoreTests) {
    & (Join-Path $PSScriptRoot 'Test-Core.ps1')
}

$targetName = switch ($Target) {
    'Editor' { 'AuthorityArenaEditor' }
    'Game' { 'AuthorityArena' }
    'Server' { 'AuthorityArenaServer' }
}

$arguments = @(
    $targetName,
    'Win64',
    $Configuration,
    "-Project=$projectPath",
    '-WaitMutex',
    '-NoHotReloadFromIDE'
)

Write-Output "UBT target=$targetName platform=Win64 configuration=$Configuration engine=$($ue.Version)"
& $ue.BuildBat @arguments
if ($LASTEXITCODE -ne 0) {
    throw "UBT failed for $targetName Win64 $Configuration with exit code $LASTEXITCODE."
}
