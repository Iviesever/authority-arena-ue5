[CmdletBinding()]
param(
    [ValidateSet('Development', 'Shipping')]
    [string] $Configuration = 'Shipping',

    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectPath = (Resolve-Path -LiteralPath (Join-Path $repositoryRoot 'AuthorityArena.uproject')).Path
$ue = & (Join-Path $PSScriptRoot 'Find-UE58.ps1')
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
$workingTreeDirty = -not [string]::IsNullOrWhiteSpace(
    ((& git -C $repositoryRoot status --porcelain) -join "`n"))
if ($workingTreeDirty) {
    throw 'Refusing to package a dirty working tree.'
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $OutputDirectory = Join-Path $repositoryRoot "Artifacts\package\$($sourceSha.Substring(0, 8))-$stamp"
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$artifactsRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts')).TrimEnd('\') + '\'
if (-not $OutputDirectory.StartsWith($artifactsRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Package output must stay inside $artifactsRoot"
}
if (Test-Path -LiteralPath $OutputDirectory) {
    throw "Package output already exists; refusing to overwrite: $OutputDirectory"
}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$uatLog = Join-Path $OutputDirectory 'BuildCookRun.log'

& (Join-Path $PSScriptRoot 'Test-Core.ps1')

$arguments = @(
    'BuildCookRun',
    "-project=$projectPath",
    '-noP4',
    '-unattended',
    '-platform=Win64',
    "-clientconfig=$Configuration",
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-iostore',
    '-archive',
    "-archivedirectory=$OutputDirectory",
    '-utf8output'
)
& $ue.RunUat @arguments 2>&1 | Tee-Object -FilePath $uatLog
$uatExitCode = $LASTEXITCODE
if ($uatExitCode -ne 0) {
    throw "BuildCookRun failed with exit code $uatExitCode; see $uatLog"
}

$allFiles = @(Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File)
$mainExecutable = $allFiles |
    Where-Object { $_.Name -eq 'AuthorityArena.exe' -or $_.Name -eq 'AuthorityArena-Win64-Shipping.exe' } |
    Sort-Object { $_.Name -eq 'AuthorityArena.exe' } -Descending |
    Select-Object -First 1
if ($null -eq $mainExecutable) {
    throw "BuildCookRun succeeded but no AuthorityArena executable exists under $OutputDirectory"
}
$pakFiles = @($allFiles | Where-Object Extension -eq '.pak')
$utocFiles = @($allFiles | Where-Object Extension -eq '.utoc')
$ucasFiles = @($allFiles | Where-Object Extension -eq '.ucas')
if ($pakFiles.Count -eq 0) {
    throw 'BuildCookRun produced no Pak file.'
}
if ($utocFiles.Count -eq 0 -or $ucasFiles.Count -eq 0) {
    throw 'IoStore is enabled but .utoc/.ucas output is missing.'
}

$manifestPath = Join-Path $OutputDirectory 'package-manifest.json'
$manifest = [ordered]@{
    schemaVersion = 1
    result = 'PASS'
    sourceSha = $sourceSha
    workingTreeDirty = $false
    engineVersion = $ue.Version
    engineChangelist = $ue.Changelist
    configuration = $Configuration
    outputDirectory = $OutputDirectory
    mainExecutable = $mainExecutable.FullName
    mainExecutableSha256 = (Get-FileHash -LiteralPath $mainExecutable.FullName -Algorithm SHA256).Hash
    totalBytes = [long](($allFiles | Measure-Object Length -Sum).Sum)
    fileCount = $allFiles.Count
    pakCount = $pakFiles.Count
    utocCount = $utocFiles.Count
    ucasCount = $ucasFiles.Count
    ioStoreEnabled = $true
    stages = [ordered]@{
        build = $true
        cook = $true
        stage = $true
        pak = $true
        ioStore = $true
        archive = $true
    }
    uatLog = $uatLog
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM
& (Join-Path $PSScriptRoot 'Verify-PackagedBuild.ps1') `
    -ManifestPath $manifestPath -ExpectedSourceSha $sourceSha
Write-Output "PASS package manifest=$manifestPath"
