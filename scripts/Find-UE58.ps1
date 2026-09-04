[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$candidates = [System.Collections.Generic.List[string]]::new()

$registryPaths = @(
    'HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8',
    'HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds'
)

if (Test-Path -LiteralPath $registryPaths[0]) {
    $installedDirectory = (Get-ItemProperty -LiteralPath $registryPaths[0]).InstalledDirectory
    if ($installedDirectory) {
        $candidates.Add($installedDirectory)
    }
}

if (Test-Path -LiteralPath $registryPaths[1]) {
    $properties = Get-ItemProperty -LiteralPath $registryPaths[1]
    foreach ($property in $properties.PSObject.Properties) {
        if ($property.Name -notmatch '^PS' -and $property.Value -is [string]) {
            $candidates.Add($property.Value)
        }
    }
}

$candidates.Add('D:\program\UnrealEngine\Epic Games\UE_5.8')
$candidates.Add('C:\Program Files\Epic Games\UE_5.8')

foreach ($candidate in $candidates | Select-Object -Unique) {
    $versionPath = Join-Path $candidate 'Engine\Build\Build.version'
    if (-not (Test-Path -LiteralPath $versionPath -PathType Leaf)) {
        continue
    }

    $version = Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json
    if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) {
        continue
    }

    $result = [pscustomobject]@{
        Root = $candidate
        Version = "$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)"
        Changelist = [long]$version.Changelist
        Editor = Join-Path $candidate 'Engine\Binaries\Win64\UnrealEditor.exe'
        EditorCmd = Join-Path $candidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        BuildBat = Join-Path $candidate 'Engine\Build\BatchFiles\Build.bat'
        RunUat = Join-Path $candidate 'Engine\Build\BatchFiles\RunUAT.bat'
    }

    foreach ($requiredPath in @($result.Editor, $result.EditorCmd, $result.BuildBat, $result.RunUat)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "UE 5.8 installation is incomplete; missing $requiredPath"
        }
    }
    return $result
}

throw 'Unable to locate a complete Unreal Engine 5.8 installation.'
