param([switch]$NoBrowser)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root

function Stop-WithError([string]$message) {
    Write-Host ""
    Write-Host "ERROR: $message" -ForegroundColor Red
    exit 1
}

function Read-EnvFile([string]$path) {
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $path) {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith('#')) { continue }
        $parts = $trimmed.Split('=', 2)
        if ($parts.Count -eq 2) { $values[$parts[0].Trim()] = $parts[1].Trim() }
    }
    return $values
}

function New-LocalDatabasePassword {
    $bytes = New-Object byte[] 24
    $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        $rng.GetBytes($bytes)
    } finally {
        $rng.Dispose()
    }
    return [Convert]::ToBase64String($bytes).TrimEnd('=').Replace('+', '-').Replace('/', '_')
}

function Convert-ToBase64Utf8([string]$value) {
    return [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($value))
}

function Get-LanIPv4 {
    $preferred = Get-NetIPConfiguration -ErrorAction SilentlyContinue |
        Where-Object { $_.NetAdapter.Status -eq 'Up' -and $_.IPv4DefaultGateway -and $_.IPv4Address } |
        ForEach-Object { $_.IPv4Address.IPAddress } |
        Where-Object { $_ -and $_ -notlike '169.254.*' -and $_ -ne '127.0.0.1' } |
        Select-Object -First 1
    if ($preferred) { return $preferred }

    return Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -notlike '169.254.*' -and $_.IPAddress -ne '127.0.0.1' } |
        Select-Object -ExpandProperty IPAddress -First 1
}

try { & docker --version | Out-Host } catch { Stop-WithError "Docker was not found. Install Docker Desktop first." }
if ($LASTEXITCODE -ne 0) { Stop-WithError "Docker was not found. Install Docker Desktop first." }

& docker info *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError "Docker Desktop is not running. Start it, wait until it is ready, then run this file again." }

# Advisory only: final image and data sizes require measurement after publishing.
try {
    $disk = Get-PSDrive -Name ([System.IO.Path]::GetPathRoot($root).Substring(0, 1))
    if ($disk.Free -lt 8GB) { Write-Warning 'Less than 8 GiB free for image downloads and data.' }
    $memory = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
    if ($memory -lt 4GB) { Write-Warning 'Less than 4 GiB RAM; the five services may be tight.' }
} catch { Write-Warning 'Could not read disk or memory capacity; continuing.' }

# Refuse automatic credential generation when an old named volume exists.
# Restore the matching .env rather than giving an existing database a new password.
$exampleValues = Read-EnvFile '.env.example'
$existingValues = if (Test-Path -LiteralPath '.env') { Read-EnvFile '.env' } else { @{} }
$volumeProject = if ($existingValues['COMPOSE_PROJECT_NAME']) { $existingValues['COMPOSE_PROJECT_NAME'] } else { $exampleValues['COMPOSE_PROJECT_NAME'] }
if (-not $volumeProject) { $volumeProject = 'tongdou-v010' }
foreach ($entry in @(@('MYSQL_ROOT_PASSWORD', 'mysql-data'), @('VOICEPRINT_API_TOKEN', 'voiceprint-data'))) {
    $credential = $existingValues[$entry[0]]
    if (-not $credential -or $credential -eq 'GENERATED_ON_FIRST_START') {
        & docker volume inspect "$volumeProject-$($entry[1])" *> $null
        if ($LASTEXITCODE -eq 0) {
            Stop-WithError "A credential-bearing volume exists but $($entry[0]) is missing. Restore the matching .env before starting."
        }
    }
}

if (-not (Test-Path -LiteralPath '.env')) {
    if (Test-Path -LiteralPath 'data\.config.yaml') {
        Stop-WithError 'Existing voice configuration found without .env. Restore the matching .env before starting.'
    }
    $envContent = Get-Content -Raw -LiteralPath '.env.example'
    $generatedDbPassword = New-LocalDatabasePassword
    $envContent = [regex]::Replace(
        $envContent,
        '(?m)^MYSQL_ROOT_PASSWORD=.*$',
        "MYSQL_ROOT_PASSWORD=$generatedDbPassword"
    )
    [System.IO.File]::WriteAllText(
        (Join-Path $root '.env'),
        $envContent,
        [System.Text.UTF8Encoding]::new($false)
    )
    Write-Host "Created .env with a random local database password. Existing .env files are never overwritten."
}

$envValues = Read-EnvFile '.env'
if (-not $envValues['VOICEPRINT_API_TOKEN'] -or $envValues['VOICEPRINT_API_TOKEN'] -eq 'GENERATED_ON_FIRST_START') {
    $voiceprintToken = New-LocalDatabasePassword
    $envText = Get-Content -Raw -LiteralPath '.env'
    if ($envText -match '(?m)^VOICEPRINT_API_TOKEN=') {
        $envText = [regex]::Replace($envText, '(?m)^VOICEPRINT_API_TOKEN=.*$', "VOICEPRINT_API_TOKEN=$voiceprintToken")
    } else {
        $envText += "`nVOICEPRINT_API_TOKEN=$voiceprintToken`n"
    }
    [System.IO.File]::WriteAllText((Join-Path $root '.env'), $envText, [System.Text.UTF8Encoding]::new($false))
    $envValues = Read-EnvFile '.env'
}
if ($envValues['VOICEPRINT_API_TOKEN'] -notmatch '^[A-Za-z0-9_-]{32,}$') {
    Stop-WithError 'VOICEPRINT_API_TOKEN must have at least 32 letters, numbers, underscores or hyphens.'
}
$version = $envValues['TONGDOU_VERSION']
if ($version -notmatch '^v?[0-9]+\.[0-9]+\.[0-9]+([-.][A-Za-z0-9.-]+)?$') {
    Stop-WithError 'Set TONGDOU_VERSION in .env to a fixed published image version.'
}
$imageNamespace = $envValues['TONGDOU_IMAGE_NAMESPACE']
if (-not $imageNamespace) { $imageNamespace = 'tongdou' }
if ($imageNamespace -notmatch '^[a-z0-9][a-z0-9._/-]*$') {
    Stop-WithError 'TONGDOU_IMAGE_NAMESPACE is invalid.'
}
# Compose normally gives exported variables priority over .env. Keep this package's
# credential and image settings authoritative, as the Linux launcher does.
foreach ($name in @($envValues.Keys) + @('COMPOSE_FILE', 'COMPOSE_PROJECT_NAME', 'COMPOSE_PROFILES', 'COMPOSE_ENV_FILES', 'COMPOSE_DISABLE_ENV_FILE')) {
    Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue
}
$project = if ($envValues['COMPOSE_PROJECT_NAME']) { $envValues['COMPOSE_PROJECT_NAME'] } else { 'tongdou-v010' }
$dashboardPort = if ($envValues['TONGDOU_DASHBOARD_PORT']) { [int]$envValues['TONGDOU_DASHBOARD_PORT'] } else { 8102 }
$wsPort = if ($envValues['TONGDOU_WS_PORT']) { [int]$envValues['TONGDOU_WS_PORT'] } else { 8100 }
$httpPort = if ($envValues['TONGDOU_HTTP_PORT']) { [int]$envValues['TONGDOU_HTTP_PORT'] } else { 8103 }
$dbPassword = $envValues['MYSQL_ROOT_PASSWORD']
$dbName = if ($envValues['MYSQL_DATABASE']) { $envValues['MYSQL_DATABASE'] } else { 'xiaozhi_esp32_server' }
if (-not $dbPassword) { Stop-WithError "MYSQL_ROOT_PASSWORD is missing from .env." }
if ($dbPassword -eq 'GENERATED_ON_FIRST_START') {
    Stop-WithError "MYSQL_ROOT_PASSWORD still contains the template marker. Delete .env and run START_TONGDOU_SERVER.bat again."
}

$lanIp = Get-LanIPv4
if (-not $lanIp) { Stop-WithError "No active LAN IPv4 address was found. Connect this PC to the same network as TongDou." }

Write-Host ""
Write-Host "Pulling published images and starting database, cache, and dashboard..." -ForegroundColor Cyan
& docker compose --env-file .env -p $project pull
if ($LASTEXITCODE -ne 0) { Stop-WithError "Docker could not pull the published images." }
& docker compose --env-file .env -p $project up -d --no-build tongdou-db tongdou-redis tongdou-manager
if ($LASTEXITCODE -ne 0) { Stop-WithError "Docker could not start the database/dashboard services." }

$secret = $null
for ($attempt = 1; $attempt -le 300; $attempt++) {
    $savedErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = 'SilentlyContinue'
    $secretResult = & docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot -N -B $dbName -e "SELECT param_value FROM sys_params WHERE param_code='server.secret' LIMIT 1;" 2>$null
    $queryExitCode = $LASTEXITCODE
    $ErrorActionPreference = $savedErrorPreference
    if ($queryExitCode -eq 0 -and $secretResult) {
        $candidate = ($secretResult | Select-Object -First 1).Trim()
        if ($candidate -and $candidate -ne 'null') { $secret = $candidate; break }
    }
    Start-Sleep -Seconds 3
}
if (-not $secret) { Stop-WithError "The dashboard did not finish database initialization. Run STATUS_TONGDOU_SERVER.bat and check Docker Desktop logs." }

Get-Content -Raw -LiteralPath 'main/xiaozhi-server/mysql-init/01-voiceprint.sql' | & docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError 'Could not initialize the voiceprint database.' }

$hasQWeatherKey = $envValues.ContainsKey('TONGDOU_QWEATHER_API_KEY')
$hasQWeatherHost = $envValues.ContainsKey('TONGDOU_QWEATHER_API_HOST')
if ($hasQWeatherKey -xor $hasQWeatherHost) {
    Stop-WithError "Set both TONGDOU_QWEATHER_API_KEY and TONGDOU_QWEATHER_API_HOST in .env, or leave both commented out."
}

$sharedToolSql = New-Object System.Collections.Generic.List[string]
if ($hasQWeatherKey -and $hasQWeatherHost) {
    $weatherKey64 = Convert-ToBase64Utf8 $envValues['TONGDOU_QWEATHER_API_KEY']
    $weatherHost64 = Convert-ToBase64Utf8 $envValues['TONGDOU_QWEATHER_API_HOST']
    $sharedToolSql.Add("UPDATE ai_model_provider SET fields=JSON_SET(fields, '`$[0].default', CONVERT(FROM_BASE64('$weatherKey64') USING utf8mb4), '`$[2].default', CONVERT(FROM_BASE64('$weatherHost64') USING utf8mb4)) WHERE id='SYSTEM_PLUGIN_WEATHER';")
    $sharedToolSql.Add("UPDATE ai_agent_plugin_mapping SET param_info=JSON_SET(param_info, '`$.api_key', CONVERT(FROM_BASE64('$weatherKey64') USING utf8mb4), '`$.api_host', CONVERT(FROM_BASE64('$weatherHost64') USING utf8mb4)) WHERE plugin_id='SYSTEM_PLUGIN_WEATHER';")
}

if ($envValues.ContainsKey('TONGDOU_TAVILY_API_KEY')) {
    $tavilyKey64 = Convert-ToBase64Utf8 $envValues['TONGDOU_TAVILY_API_KEY']
    $sharedToolSql.Add("UPDATE ai_model_provider SET fields=JSON_SET(fields, '`$[0].default', 'tavily', '`$[3].default', CONVERT(FROM_BASE64('$tavilyKey64') USING utf8mb4)) WHERE id='SYSTEM_PLUGIN_WEB_SEARCH';")
    $sharedToolSql.Add("UPDATE ai_agent_plugin_mapping SET param_info=JSON_SET(param_info, '`$.provider', 'tavily', '`$.api_key', CONVERT(FROM_BASE64('$tavilyKey64') USING utf8mb4)) WHERE plugin_id='SYSTEM_PLUGIN_WEB_SEARCH';")
}

if ($sharedToolSql.Count -gt 0) {
    ($sharedToolSql -join "`n") | & docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot $dbName *> $null
    if ($LASTEXITCODE -ne 0) { Stop-WithError "Could not apply the optional Early Tester tool credentials." }
    Write-Host "Applied optional Early Tester tool credentials from the local .env file. Values were not printed."
}

$voiceprintEndpoint64 = Convert-ToBase64Utf8 ("http://voiceprint-api:8005/voiceprint/health?key=" + $envValues['VOICEPRINT_API_TOKEN'])
$voiceprintSql = "UPDATE sys_params SET param_value=CONVERT(FROM_BASE64('$voiceprintEndpoint64') USING utf8mb4) WHERE param_code='server.voice_print' AND (param_value IS NULL OR param_value='' OR param_value='null' OR param_value LIKE 'http://voiceprint-api:8005/%');"
$voiceprintSql | & docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot $dbName *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError 'Could not configure the voiceprint endpoint.' }
& docker compose --env-file .env -p $project exec -T tongdou-redis redis-cli HDEL 'sys:params' 'server.voice_print' *> $null

$websocketUrl = "ws://${lanIp}:${wsPort}/xiaozhi/v1/"
$otaUrl = "http://${lanIp}:${dashboardPort}/xiaozhi/ota/"
$resourceUrl = "http://${lanIp}:${httpPort}/tongdou/resources"
$sql = "UPDATE sys_params SET param_value='$websocketUrl' WHERE param_code='server.websocket'; UPDATE sys_params SET param_value='$otaUrl' WHERE param_code='server.ota';"
& docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot $dbName -e $sql *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError "Could not save the LAN service addresses in the TongDou database." }

& docker compose --env-file .env -p $project exec -T tongdou-redis redis-cli HDEL 'sys:params' 'server.websocket' 'server.ota' *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError "Could not clear the old TongDou address cache." }

$config = @"
server:
  ip: 0.0.0.0
  port: 8000
  http_port: 8003

manager-api:
  url: http://tongdou-manager:8003/xiaozhi/
  secret: $secret

tongdou_locale_resources:
  base_url: $resourceUrl
  directory: config/assets/tongdou_locale_packs
"@
[System.IO.File]::WriteAllText((Join-Path $root 'data\.config.yaml'), $config, [System.Text.UTF8Encoding]::new($false))

if (-not (Test-Path -LiteralPath 'data\.mcp_server_settings.json')) {
    [System.IO.File]::WriteAllText((Join-Path $root 'data\.mcp_server_settings.json'), "{`n  `"mcpServers`": {}`n}`n", [System.Text.UTF8Encoding]::new($false))
}

Write-Host "Starting TongDou voice service..." -ForegroundColor Cyan
& docker compose --env-file .env -p $project up -d --no-build
if ($LASTEXITCODE -ne 0) { Stop-WithError "Docker could not start all TongDou services." }

 $dashboardHealthy = $false
for ($attempt = 1; $attempt -le 30; $attempt++) {
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri "http://localhost:${dashboardPort}/" -TimeoutSec 3
        if ($response.StatusCode -eq 200) { $dashboardHealthy = $true; break }
    } catch { }
    Start-Sleep -Seconds 2
}

Write-Host ""
if (-not $dashboardHealthy) { Stop-WithError "Dashboard did not return HTTP 200." }
Write-Host "SUCCESS: TongDou Server is running." -ForegroundColor Green
Write-Host ""
Write-Host "Local dashboard:"
Write-Host "http://localhost:${dashboardPort}"
Write-Host ""
Write-Host "TongDou device Server address:"
Write-Host "http://${lanIp}:${dashboardPort}" -ForegroundColor Yellow
Write-Host ""
Write-Host "Voice WebSocket: $websocketUrl"
Write-Host "Language resources: $resourceUrl"
Write-Host "If Windows Firewall asks, allow access on Private networks."

if (-not $NoBrowser) { Start-Process "http://localhost:${dashboardPort}" }
exit 0
