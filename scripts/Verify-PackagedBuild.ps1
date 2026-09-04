[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ManifestPath,

    [string] $ExpectedSourceSha
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw "Package manifest does not exist: $ManifestPath"
}
$manifestPathResolved = (Resolve-Path -LiteralPath $ManifestPath).Path
$manifest = Get-Content -LiteralPath $manifestPathResolved -Raw | ConvertFrom-Json

if ($manifest.schemaVersion -ne 1 -or $manifest.result -ne 'PASS') {
    throw "Invalid package manifest schema/result: schema=$($manifest.schemaVersion) result=$($manifest.result)"
}
if ($manifest.workingTreeDirty -ne $false) {
    throw 'Package was not built from a clean working tree.'
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceSha) -and
    $manifest.sourceSha -ne $ExpectedSourceSha) {
    throw "Package source mismatch: expected=$ExpectedSourceSha actual=$($manifest.sourceSha)"
}
foreach ($stage in @('build', 'cook', 'stage', 'pak', 'archive')) {
    if ($manifest.stages.$stage -ne $true) {
        throw "Package stage did not pass: $stage"
    }
}
if ($manifest.ioStoreEnabled -and $manifest.stages.ioStore -ne $true) {
    throw 'IoStore is enabled but no successful IoStore stage was recorded.'
}

$executable = $manifest.mainExecutable
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Packaged executable is missing: $executable"
}
$actualHash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
if ($actualHash -ne $manifest.mainExecutableSha256) {
    throw "Packaged executable SHA-256 mismatch: expected=$($manifest.mainExecutableSha256) actual=$actualHash"
}
if ($manifest.totalBytes -le 0 -or $manifest.fileCount -le 0 -or
    $manifest.pakCount -lt 1) {
    throw 'Package manifest has invalid size/file/Pak counts.'
}

Write-Output "PASS packaged-build sha=$($manifest.sourceSha) exe_sha256=$actualHash bytes=$($manifest.totalBytes)"
