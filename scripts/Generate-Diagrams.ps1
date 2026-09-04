[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repositoryRoot 'docs\images'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$palette = @{
    Background = [System.Drawing.Color]::FromArgb(255, 10, 14, 25)
    Surface = [System.Drawing.Color]::FromArgb(255, 28, 36, 54)
    Surface2 = [System.Drawing.Color]::FromArgb(255, 38, 49, 70)
    Text = [System.Drawing.Color]::FromArgb(255, 232, 239, 250)
    Muted = [System.Drawing.Color]::FromArgb(255, 165, 180, 202)
    Cyan = [System.Drawing.Color]::FromArgb(255, 77, 211, 241)
    Blue = [System.Drawing.Color]::FromArgb(255, 69, 125, 255)
    Orange = [System.Drawing.Color]::FromArgb(255, 255, 139, 66)
    Green = [System.Drawing.Color]::FromArgb(255, 81, 214, 151)
    Red = [System.Drawing.Color]::FromArgb(255, 255, 100, 112)
}

$titleFont = [System.Drawing.Font]::new('Segoe UI Semibold', 28)
$subtitleFont = [System.Drawing.Font]::new('Segoe UI', 13)
$boxTitleFont = [System.Drawing.Font]::new('Segoe UI Semibold', 16)
$bodyFont = [System.Drawing.Font]::new('Consolas', 11)
$smallFont = [System.Drawing.Font]::new('Segoe UI', 10)

function New-ArrowPen {
    param([System.Drawing.Color] $Color)
    $pen = [System.Drawing.Pen]::new($Color, 3)
    $pen.CustomEndCap = [System.Drawing.Drawing2D.AdjustableArrowCap]::new(5, 6)
    return $pen
}

function Draw-Box {
    param(
        [System.Drawing.Graphics] $Graphics,
        [System.Drawing.RectangleF] $Rect,
        [string] $Title,
        [string[]] $Lines,
        [System.Drawing.Color] $Accent
    )
    $Graphics.FillRectangle([System.Drawing.SolidBrush]::new($palette.Surface), $Rect)
    $Graphics.FillRectangle([System.Drawing.SolidBrush]::new($Accent), $Rect.X, $Rect.Y, 7, $Rect.Height)
    $Graphics.DrawRectangle([System.Drawing.Pen]::new($palette.Surface2, 2), $Rect.X, $Rect.Y, $Rect.Width, $Rect.Height)
    $Graphics.DrawString($Title, $boxTitleFont, [System.Drawing.SolidBrush]::new($palette.Text), $Rect.X + 20, $Rect.Y + 15)
    $y = $Rect.Y + 50
    foreach ($line in $Lines) {
        $Graphics.DrawString($line, $bodyFont, [System.Drawing.SolidBrush]::new($palette.Muted), $Rect.X + 20, $y)
        $y += 24
    }
}

function Draw-Arrow {
    param(
        [System.Drawing.Graphics] $Graphics,
        [float] $X1,
        [float] $Y1,
        [float] $X2,
        [float] $Y2,
        [string] $Label,
        [System.Drawing.Color] $Color
    )
    $pen = New-ArrowPen $Color
    try {
        $Graphics.DrawLine($pen, $X1, $Y1, $X2, $Y2)
    }
    finally {
        $pen.Dispose()
    }
    if (-not [string]::IsNullOrWhiteSpace($Label)) {
        $labelBrush = [System.Drawing.SolidBrush]::new($Color)
        $Graphics.DrawString($Label, $smallFont, $labelBrush, [Math]::Min($X1, $X2) + [Math]::Abs($X2 - $X1) / 2 - 55, [Math]::Min($Y1, $Y2) + [Math]::Abs($Y2 - $Y1) / 2 - 20)
        $labelBrush.Dispose()
    }
}

function New-Diagram {
    param([string] $Filename, [string] $Title, [string] $Subtitle, [scriptblock] $Draw)
    $bitmap = [System.Drawing.Bitmap]::new(1600, 900)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
        $graphics.Clear($palette.Background)
        $graphics.DrawString($Title, $titleFont, [System.Drawing.SolidBrush]::new($palette.Text), 55, 38)
        $graphics.DrawString($Subtitle, $subtitleFont, [System.Drawing.SolidBrush]::new($palette.Muted), 58, 87)
        & $Draw $graphics
        $path = Join-Path $outputDirectory $Filename
        $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        return $path
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$outputs = @()
$outputs += New-Diagram 'architecture.png' 'AuthorityArena architecture' 'UE 5.8 C++ ownership, process topology, build and evidence boundaries' {
    param($g)
    Draw-Box $g ([Drawing.RectangleF]::new(55, 155, 330, 220)) 'PowerShell orchestration' @(
        'unique UDP port + RunId',
        'owned PID/start/path',
        'network/failure matrix',
        'JSONL + report verifier'
    ) $palette.Cyan
    Draw-Box $g ([Drawing.RectangleF]::new(490, 140, 610, 255)) 'Authority process // UnrealEditor-Cmd -server' @(
        'AAuthorityArenaGameMode  (server only)',
        'AAuthorityArenaGameState (replicated)',
        'AAuthorityArenaPlayerState + ASC/Attributes',
        'Projectile / Damage / Death / Score / Respawn'
    ) $palette.Orange
    Draw-Box $g ([Drawing.RectangleF]::new(1210, 145, 330, 220)) 'Client processes x2' @(
        'AutonomousProxy input',
        'SimulatedProxy peer',
        'predicted GAS + HUD',
        'RepNotify + correction'
    ) $palette.Blue
    Draw-Box $g ([Drawing.RectangleF]::new(490, 510, 610, 220)) 'AuthorityArenaCore // standard C++' @(
        'DecisionCode + fail-closed rules',
        'NetworkScenario + report model',
        'MQB 5.4: build + 41 assertions',
        'also compiled by UBT'
    ) $palette.Green
    Draw-Box $g ([Drawing.RectangleF]::new(55, 535, 330, 180)) 'Build / delivery' @(
        'MQB -> Core',
        'UBT -> UE targets',
        'RunUAT -> Cook/Package',
        'Artifacts ignored'
    ) $palette.Green
    Draw-Box $g ([Drawing.RectangleF]::new(1210, 535, 330, 180)) 'Evidence' @(
        '3 process JSONL streams',
        'Automation + screenshots',
        'source/artifact SHA-256',
        'source-only Release'
    ) $palette.Cyan
    Draw-Arrow $g 385 260 490 260 'launch' $palette.Cyan
    Draw-Arrow $g 1100 260 1210 260 'replication / RPC' $palette.Blue
    Draw-Arrow $g 795 395 795 510 'validation calls' $palette.Green
    Draw-Arrow $g 385 625 490 625 'toolchain' $palette.Green
    Draw-Arrow $g 1100 625 1210 625 'verified output' $palette.Cyan
}

$outputs += New-Diagram 'replication-flow.png' 'Replication and authority flow' 'Who may write each state, and how client observation converges' {
    param($g)
    Draw-Box $g ([Drawing.RectangleF]::new(55, 165, 360, 190)) 'Client1 // AutonomousProxy' @(
        'local movement + input',
        'TryActivateAbilitiesByTag',
        'Prediction Key / saved moves',
        'never writes final state'
    ) $palette.Blue
    Draw-Box $g ([Drawing.RectangleF]::new(620, 145, 360, 240)) 'Server // Authority' @(
        'ownership + lifecycle checks',
        'Core validation / GAS cost',
        'spawn Projectile + hit',
        'Health / Score / Respawn truth'
    ) $palette.Orange
    Draw-Box $g ([Drawing.RectangleF]::new(1185, 165, 360, 190)) 'Client2 // Simulated peer' @(
        'replicated CharacterMovement',
        'PlayerState + GameState',
        'Attribute RepNotify',
        'presentation-only multicast'
    ) $palette.Blue
    Draw-Box $g ([Drawing.RectangleF]::new(205, 530, 420, 180)) 'Persistent PlayerState' @(
        'ConnectionId / Score / Deaths',
        'ASC Mixed replication',
        'Health / Energy / Tags',
        'survives Pawn replacement'
    ) $palette.Green
    Draw-Box $g ([Drawing.RectangleF]::new(975, 530, 420, 180)) 'Disposable Character' @(
        'current movement avatar',
        'camera + local input',
        'Combat / Health components',
        'destroy -> respawn'
    ) $palette.Cyan
    Draw-Arrow $g 415 250 620 250 'input / RPC' $palette.Blue
    Draw-Arrow $g 980 250 1185 250 'properties / RepNotify' $palette.Orange
    Draw-Arrow $g 620 320 415 320 'confirm / reject / correct' $palette.Red
    Draw-Arrow $g 800 385 625 530 'owns truth' $palette.Green
    Draw-Arrow $g 800 385 975 530 'possesses avatar' $palette.Cyan
}

$outputs += New-Diagram 'gas-flow.png' 'Gameplay Ability System flow' 'Native abilities, prediction, server validation and authoritative results' {
    param($g)
    Draw-Box $g ([Drawing.RectangleF]::new(55, 160, 330, 190)) 'Client input' @(
        'Dash / Attack / Shield',
        'ASC TryActivate by tag',
        'LocalPredicted',
        'Prediction Key'
    ) $palette.Blue
    Draw-Box $g ([Drawing.RectangleF]::new(495, 150, 330, 215)) 'GAS commit' @(
        'native Cost GameplayEffect',
        'native Cooldown tag/effect',
        'Dead / Stunned blocks',
        'spec lifecycle on PlayerState'
    ) $palette.Cyan
    Draw-Box $g ([Drawing.RectangleF]::new(935, 135, 610, 245)) 'Server validation / result' @(
        'Dash: confirm root motion or reject -> correction',
        'Attack: server-only Projectile spawn / overlap',
        'Shield: State.Shield.Active + Energy cost',
        'Damage: 34 raw -> 17 shielded -> Health'
    ) $palette.Orange
    Draw-Box $g ([Drawing.RectangleF]::new(150, 550, 360, 180)) 'Predicted presentation' @(
        'DashPredicted',
        'ShieldPredicted',
        'client cost/cooldown preview',
        'HUD recent events'
    ) $palette.Blue
    Draw-Box $g ([Drawing.RectangleF]::new(620, 535, 360, 210)) 'Authoritative state' @(
        'replicated Attributes / Tags',
        'Death + Score',
        'weak-timer Respawn',
        'post-respawn snapshot'
    ) $palette.Green
    Draw-Box $g ([Drawing.RectangleF]::new(1090, 550, 360, 180)) 'Evidence boundary' @(
        'ProjectileImpact multicast only',
        'server/client JSONL',
        'Combat + DashRejected',
        'final convergence'
    ) $palette.Cyan
    Draw-Arrow $g 385 255 495 255 'activate' $palette.Blue
    Draw-Arrow $g 825 255 935 255 'server RPC via GAS' $palette.Orange
    Draw-Arrow $g 660 365 350 550 'predict' $palette.Blue
    Draw-Arrow $g 1240 380 800 535 'apply truth' $palette.Green
    Draw-Arrow $g 980 640 1090 640 'observe' $palette.Cyan
    Draw-Arrow $g 935 320 825 320 'reject / rollback' $palette.Red
}

$outputs | ForEach-Object {
    $item = Get-Item -LiteralPath $_
    if ($item.Length -lt 50000) {
        throw "Generated diagram is unexpectedly small: $($item.FullName)"
    }
    Write-Output "PASS diagram path=$($item.FullName) bytes=$($item.Length)"
}
