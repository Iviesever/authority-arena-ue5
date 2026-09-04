[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$taskRoot = Join-Path $repositoryRoot 'tasks\20260904-224034-authority-arena-0.1'

$requiredFiles = @(
    'AGENTS.md',
    '.agents\AGENTS.md',
    '.gitignore',
    'docs\PRODUCT_CONTRACT.md',
    'docs\ARCHITECTURE.md',
    'docs\ACCEPTANCE_MATRIX.md',
    'tasks\20260904-224034-authority-arena-0.1\goal-objective.md',
    'tasks\20260904-224034-authority-arena-0.1\issue.md',
    'tasks\20260904-224034-authority-arena-0.1\implementation_plan.md',
    'tasks\20260904-224034-authority-arena-0.1\task.md',
    'tasks\20260904-224034-authority-arena-0.1\progress.md',
    'tasks\20260904-224034-authority-arena-0.1\handoff.md'
)

$missingFiles = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $repositoryRoot $_) -PathType Leaf)
})
if ($missingFiles.Count -gt 0) {
    throw "Missing required contract files: $($missingFiles -join ', ')"
}

$goalPath = Join-Path $taskRoot 'goal-objective.md'
$goalHash = (Get-FileHash -LiteralPath $goalPath -Algorithm SHA256).Hash
if ([string]::IsNullOrWhiteSpace($goalHash)) {
    throw 'The copied goal has no SHA-256.'
}

$gitignore = Get-Content -LiteralPath (Join-Path $repositoryRoot '.gitignore') -Raw
foreach ($pattern in @('Binaries/', 'Intermediate/', 'Saved/', 'Artifacts/', '.mqb/')) {
    if (-not $gitignore.Contains($pattern, [StringComparison]::Ordinal)) {
        throw ".gitignore is missing required pattern: $pattern"
    }
}

$matrix = Get-Content -LiteralPath (Join-Path $repositoryRoot 'docs\ACCEPTANCE_MATRIX.md') -Raw
foreach ($pact in @('PACT-00', 'PACT-10', 'PACT-20', 'PACT-30', 'PACT-40', 'PACT-50', 'PACT-60', 'PACT-70')) {
    if (-not $matrix.Contains($pact, [StringComparison]::Ordinal)) {
        throw "Acceptance matrix is missing $pact."
    }
}
if (-not $matrix.Contains('empty custom assets', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Acceptance matrix is missing the source-only Release gate.'
}

$agentRules = Get-Content -LiteralPath (Join-Path $repositoryRoot '.agents\AGENTS.md') -Raw
foreach ($rule in @('One Agent only may write', 'RED -> minimal implementation', 'no custom assets')) {
    if (-not $agentRules.Contains($rule, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Repository workflow is missing rule: $rule"
    }
}

Write-Output "PASS contracts goal_sha256=$goalHash files=$($requiredFiles.Count)"
