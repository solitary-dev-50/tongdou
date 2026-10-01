$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

if (-not (Test-Path -LiteralPath '.env')) {
    Write-Host "TongDou Server has not been initialized in this folder." -ForegroundColor Yellow
    exit 1
}

$values = @{}
foreach ($line in Get-Content -LiteralPath '.env') {
    $parts = $line.Trim().Split('=', 2)
    if ($parts.Count -eq 2) { $values[$parts[0]] = $parts[1] }
}
$project = if ($values['COMPOSE_PROJECT_NAME']) { $values['COMPOSE_PROJECT_NAME'] } else { 'tongdou-v010' }
$dashboardPort = if ($values['TONGDOU_DASHBOARD_PORT']) { $values['TONGDOU_DASHBOARD_PORT'] } else { '8102' }

& docker compose --env-file .env -p $project ps
Write-Host ""
Write-Host "Dashboard: http://localhost:${dashboardPort}"
Write-Host "Persistent data: data, uploadfile, and Docker named MySQL/Redis volumes"
