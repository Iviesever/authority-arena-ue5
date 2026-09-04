[CmdletBinding()]
param(
    [switch] $Timings
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repositoryRoot
try {
    $sources = @(
        'Tests/AuthorityArenaCoreTests.cpp',
        'Source/AuthorityArenaCore/Private/AuthorityRules.cpp',
        'Source/AuthorityArenaCore/Private/NetworkScenario.cpp',
        'Source/AuthorityArenaCore/Private/ReportModel.cpp'
    )
    # MQB 5.4 emits /W3 as owned default policy. Passing /W4 produces MSVC D9025
    # (override warning), so the clean evidence path keeps MQB's warning level and
    # turns every emitted compiler warning into an error.
    $arguments = @('run') + $sources + @('/ISource/AuthorityArenaCore/Public', '--std', '20', '/WX')
    if ($Timings) {
        $arguments += '--timings=json'
    }

    & mqb @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "MQB core tests failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
