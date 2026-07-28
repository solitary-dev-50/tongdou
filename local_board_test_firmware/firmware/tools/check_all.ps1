param(
    [string]$Python = "python"
)

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

Invoke-Step "Verify voice model" {
    & $Python "firmware/tools/verify_voice_model.py"
}

Invoke-Step "Verify voice prompts" {
    & $Python "firmware/tools/verify_voice_prompts.py"
}

Invoke-Step "Run scenario simulator" {
    & $Python "firmware/tools/scenario_simulator.py"
}

Invoke-Step "Build firmware" {
    platformio run
} $firmwareRoot

Write-Host ""
Write-Host "OK: all checks passed."
