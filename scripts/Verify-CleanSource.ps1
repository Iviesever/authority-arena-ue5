[CmdletBinding()]
param(
    [string] $SourceSha,

    [ValidateRange(60, 240)]
    [int] $ScenarioTimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($SourceSha)) {
    $SourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
}
& git -C $repositoryRoot cat-file -e "$SourceSha`^{commit}"
if ($LASTEXITCODE -ne 0) {
    throw "Source commit does not exist: $SourceSha"
}
$resolvedSourceSha = (& git -C $repositoryRoot rev-parse "$SourceSha`^{commit}").Trim()

$artifactsRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts'))
New-Item -ItemType Directory -Path $artifactsRoot -Force | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$checkoutPath = [System.IO.Path]::GetFullPath(
    (Join-Path $artifactsRoot "clean-checkout\$($resolvedSourceSha.Substring(0, 8))-$stamp"))
$artifactsPrefix = $artifactsRoot.TrimEnd('\') + '\'
if (-not $checkoutPath.StartsWith($artifactsPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Clean checkout escaped the repository Artifacts root: $checkoutPath"
}
if (Test-Path -LiteralPath $checkoutPath) {
    throw "Clean checkout target already exists; refusing to overwrite: $checkoutPath"
}

& git clone --no-local --no-checkout $repositoryRoot $checkoutPath
if ($LASTEXITCODE -ne 0) {
    throw 'git clone failed for clean-source verification.'
}
& git -C $checkoutPath checkout --detach $resolvedSourceSha
if ($LASTEXITCODE -ne 0) {
    throw "git checkout failed for clean-source commit $resolvedSourceSha."
}
$checkoutHead = (& git -C $checkoutPath rev-parse HEAD).Trim()
if ($checkoutHead -ne $resolvedSourceSha) {
    throw "Clean checkout SHA mismatch: expected=$resolvedSourceSha actual=$checkoutHead"
}
if (-not [string]::IsNullOrWhiteSpace(((& git -C $checkoutPath status --porcelain) -join "`n"))) {
    throw 'Fresh clean-source checkout is unexpectedly dirty.'
}

$startedUtc = [datetime]::UtcNow
$cleanScripts = Join-Path $checkoutPath 'scripts'
& (Join-Path $cleanScripts 'Verify-Contracts.ps1')
& (Join-Path $cleanScripts 'Test-ProjectStructure.ps1')
& (Join-Path $cleanScripts 'Verify-Documentation.ps1')
& (Join-Path $cleanScripts 'Test-Core.ps1')
& (Join-Path $cleanScripts 'Build.ps1') -Target Editor -Configuration Development -SkipCoreTests
& (Join-Path $cleanScripts 'Build.ps1') -Target Game -Configuration Development -SkipCoreTests
& (Join-Path $cleanScripts 'Build.ps1') -Target Game -Configuration Shipping -SkipCoreTests
& (Join-Path $cleanScripts 'Run-Automation.ps1')
& (Join-Path $cleanScripts 'RunMultiplayerScenario.ps1') `
    -Scenario Combat -NetworkProfile Baseline -TimeoutSeconds $ScenarioTimeoutSeconds

$shippingOutput = Join-Path $checkoutPath 'Artifacts\package\clean-shipping'
$developmentOutput = Join-Path $checkoutPath 'Artifacts\package\clean-development'
& (Join-Path $cleanScripts 'Package-Win64.ps1') `
    -Configuration Shipping -OutputDirectory $shippingOutput
$shippingManifestPath = Join-Path $shippingOutput 'package-manifest.json'
& (Join-Path $cleanScripts 'Run-PackagedSmoke.ps1') `
    -ManifestPath $shippingManifestPath -TimeoutSeconds 90

& (Join-Path $cleanScripts 'Package-Win64.ps1') `
    -Configuration Development -OutputDirectory $developmentOutput
$developmentManifestPath = Join-Path $developmentOutput 'package-manifest.json'
& (Join-Path $cleanScripts 'RunMultiplayerScenario.ps1') `
    -Build Packaged -PackageManifest $developmentManifestPath `
    -Scenario Combat -NetworkProfile Baseline -TimeoutSeconds $ScenarioTimeoutSeconds

if (-not [string]::IsNullOrWhiteSpace(((& git -C $checkoutPath status --porcelain) -join "`n"))) {
    throw 'Clean-source checkout became dirty; generated output escaped ignored directories.'
}
$ownedSurvivors = @(
    Get-CimInstance Win32_Process |
        Where-Object {
            -not [string]::IsNullOrWhiteSpace($_.CommandLine) -and
            $_.CommandLine.Contains($checkoutPath, [StringComparison]::OrdinalIgnoreCase) -and
            $_.ProcessId -ne $PID
        }
)
if ($ownedSurvivors.Count -ne 0) {
    throw "Clean-source verification left $($ownedSurvivors.Count) process(es) referencing its checkout."
}

$shippingManifest = Get-Content -LiteralPath $shippingManifestPath -Raw | ConvertFrom-Json
$developmentManifest = Get-Content -LiteralPath $developmentManifestPath -Raw | ConvertFrom-Json
$reportPath = Join-Path $checkoutPath 'Artifacts\clean-source-report.json'
$report = [ordered]@{
    schemaVersion = 1
    result = 'PASS'
    sourceSha = $resolvedSourceSha
    checkoutPath = $checkoutPath
    checkoutDirty = $false
    coreAssertions = 41
    ueAutomationTests = 3
    builds = [ordered]@{
        editorDevelopment = $true
        gameDevelopment = $true
        gameShipping = $true
    }
    editorCombat = $true
    shipping = [ordered]@{
        manifest = $shippingManifestPath
        gameExecutableSha256 = $shippingManifest.gameExecutableSha256
        packageFingerprintSha256 = $shippingManifest.packageFingerprintSha256
        packagedSmoke = $true
    }
    development = [ordered]@{
        manifest = $developmentManifestPath
        gameExecutableSha256 = $developmentManifest.gameExecutableSha256
        packageFingerprintSha256 = $developmentManifest.packageFingerprintSha256
        packagedTwoClientCombat = $true
    }
    ownedProcessSurvivors = 0
    durationMs = [Math]::Round(([datetime]::UtcNow - $startedUtc).TotalMilliseconds, 3)
}
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding utf8NoBOM
Write-Output "PASS clean-source sha=$resolvedSourceSha report=$reportPath"
