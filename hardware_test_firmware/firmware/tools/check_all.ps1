# If Windows blocks script execution, run:
# powershell -ExecutionPolicy Bypass -File .\firmware\tools\check_all.ps1

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$firmwareRoot = Join-Path $repoRoot "firmware"

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Command,
        [string]$WorkingDirectory = $repoRoot
    )

    Write-Host ""
    Write-Host "== $Name =="
    Push-Location $WorkingDirectory
    try {
        & $Command
        if ($LASTEXITCODE -ne 0) {
            throw "$Name failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

Invoke-Step "Build firmware" {
    platformio run
} $firmwareRoot

Write-Host ""
Write-Host "OK: all checks passed."
