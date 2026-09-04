[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot

$requiredFiles = @(
    'README.md',
    'README_ZH.md',
    'docs\ARCHITECTURE.md',
    'docs\NETWORK_MODEL.md',
    'docs\GAS_DESIGN.md',
    'docs\SERVER_AUTHORITY.md',
    'docs\MULTIPROCESS_TESTING.md',
    'docs\BUILD_SYSTEM.md',
    'docs\TESTING.md',
    'docs\KNOWN_LIMITATIONS.md',
    'docs\AI_ASSISTANCE.md',
    'docs\CODE_WALKTHROUGH.md',
    'docs\INTERVIEW_GUIDE.md',
    'docs\LIVE_CHANGE_DRILLS.md',
    'docs\ROLLBACK_VS_UE_REPLICATION.md',
    'docs\RELEASE_NOTES_0.1.0.md',
    '.github\workflows\verify.yml'
)
foreach ($relativePath in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $repositoryRoot $relativePath) -PathType Leaf)) {
        throw "Missing required portfolio document: $relativePath"
    }
}

$readme = Get-Content -LiteralPath (Join-Path $repositoryRoot 'README.md') -Raw
foreach ($answer in @(
    'What it demonstrates',
    'Why it matters',
    'Run it',
    'Real multi-process evidence',
    'Predicted locally',
    'Server authoritative',
    'Known limits',
    'AI assistance'
)) {
    if (-not $readme.Contains($answer, [StringComparison]::OrdinalIgnoreCase)) {
        throw "README first-page contract is missing '$answer'."
    }
}

$aiDisclosure = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\AI_ASSISTANCE.md') -Raw
foreach ($disclosure in @(
    'The user defined the career goal, topic, deadline, scope, constraints, and acceptance direction.',
    'Codex GPT-5.6 Sol performed architecture refinement, code, tests, debugging, packaging, audit, and documentation.',
    'The user did not independently hand-write this delivery code.',
    'This work must not be represented as entirely hand-written.',
    'complete at least one Live Change Drill before an interview'
)) {
    if (-not $aiDisclosure.Contains($disclosure, [StringComparison]::Ordinal)) {
        throw "AI disclosure is missing required statement '$disclosure'."
    }
}

$interviewGuide = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\INTERVIEW_GUIDE.md') -Raw
foreach ($topic in @(
    'Network roles', 'Actor ownership', 'RPC', 'RepNotify', 'PlayerState',
    'GAS prediction', 'server rejection', 'Character Movement', 'Reliable',
    'Dormancy', 'Relevancy', 'Rollback', 'multi-process', 'process cleanup', 'failure'
)) {
    if (-not $interviewGuide.Contains($topic, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Interview guide is missing topic '$topic'."
    }
}

$drills = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\LIVE_CHANGE_DRILLS.md') -Raw
foreach ($drill in @('Energy regeneration', 'Dash rejection', 'RepNotify UI')) {
    if (-not $drills.Contains($drill, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Live Change Drills are missing '$drill'."
    }
}

$rollback = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\ROLLBACK_VS_UE_REPLICATION.md') -Raw
foreach ($dimension in @('goal', 'ownership', 'prediction', 'correction', 'bandwidth', 'determinism', 'use case')) {
    if (-not $rollback.Contains($dimension, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Rollback comparison is missing dimension '$dimension'."
    }
}

$releaseNotes = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\RELEASE_NOTES_0.1.0.md') -Raw
if (-not $releaseNotes.Contains('no custom release assets', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Release notes do not state the source-only asset policy.'
}

Write-Output "PASS documentation files=$($requiredFiles.Count)"
