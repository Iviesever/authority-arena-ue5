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
$gameExecutable = $manifest.gameExecutable
if (-not (Test-Path -LiteralPath $gameExecutable -PathType Leaf)) {
    throw "Packaged game executable is missing: $gameExecutable"
}
$actualGameHash = (Get-FileHash -LiteralPath $gameExecutable -Algorithm SHA256).Hash
if ($actualGameHash -ne $manifest.gameExecutableSha256) {
    throw "Packaged game executable SHA-256 mismatch: expected=$($manifest.gameExecutableSha256) actual=$actualGameHash"
}

$payloadFiles = @($manifest.payloadFiles)
if ($payloadFiles.Count -lt 5) {
    throw "Package payload inventory is unexpectedly short: $($payloadFiles.Count) files."
}
$outputRoot = [System.IO.Path]::GetFullPath($manifest.outputDirectory).TrimEnd('\')
$outputPrefix = $outputRoot + '\'
$verifiedPayloadBytes = [long]0
foreach ($payload in $payloadFiles) {
    $relativeWindowsPath = $payload.relativePath.Replace('/', '\')
    $payloadPath = [System.IO.Path]::GetFullPath((Join-Path $outputRoot $relativeWindowsPath))
    if (-not $payloadPath.StartsWith($outputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Package payload escapes output directory: $($payload.relativePath)"
    }
    if (-not (Test-Path -LiteralPath $payloadPath -PathType Leaf)) {
        throw "Package payload is missing: $($payload.relativePath)"
    }
    $payloadFile = Get-Item -LiteralPath $payloadPath
    if ([long]$payloadFile.Length -ne [long]$payload.bytes) {
        throw "Package payload size mismatch: $($payload.relativePath)"
    }
    $payloadHash = (Get-FileHash -LiteralPath $payloadPath -Algorithm SHA256).Hash
    if ($payloadHash -ne $payload.sha256) {
        throw "Package payload SHA-256 mismatch: $($payload.relativePath)"
    }
    $verifiedPayloadBytes += [long]$payload.bytes
}
if ($verifiedPayloadBytes -ne [long]$manifest.payloadBytes) {
    throw "Package payload byte count mismatch: expected=$($manifest.payloadBytes) actual=$verifiedPayloadBytes"
}
$fingerprintInput = ($payloadFiles | Sort-Object relativePath | ForEach-Object {
    "$($_.relativePath)|$($_.bytes)|$($_.sha256)"
}) -join "`n"
$actualFingerprint = [Convert]::ToHexString(
    [Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($fingerprintInput)))
if ($actualFingerprint -ne $manifest.packageFingerprintSha256) {
    throw "Package fingerprint mismatch: expected=$($manifest.packageFingerprintSha256) actual=$actualFingerprint"
}
if ($manifest.totalBytes -le 0 -or $manifest.fileCount -le 0 -or
    $manifest.pakCount -lt 1) {
    throw 'Package manifest has invalid size/file/Pak counts.'
}

Write-Output "PASS packaged-build sha=$($manifest.sourceSha) game_sha256=$actualGameHash package_sha256=$actualFingerprint payload_bytes=$verifiedPayloadBytes"
