# PowerShell script to generate CMSIS Pack using WSL
# Author: FoolCat
# Date: 2025-12-02

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Generating CMSIS Pack using WSL" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get current directory
$currentDir = Get-Location
$winPath = $currentDir.Path

# Convert Windows path to WSL path
# Extract drive letter and convert to lowercase
$driveLetter = $winPath.Substring(0, 1).ToLower()
$restOfPath = $winPath.Substring(2) -replace '\\', '/'
$wslPath = "/mnt/$driveLetter$restOfPath"

Write-Host "Windows Path: $winPath" -ForegroundColor Yellow
Write-Host "WSL Path: $wslPath" -ForegroundColor Yellow
Write-Host ""

# Check if WSL is installed
try {
    $wslCheck = wsl --status 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "WSL check failed"
    }
} catch {
    Write-Host "ERROR: WSL is not installed or not accessible!" -ForegroundColor Red
    Write-Host "Please install WSL first using: wsl --install" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Checking and fixing line endings..." -ForegroundColor Yellow

# Convert CRLF to LF for gen_pack_wsl.sh if needed
$genPackScript = Join-Path $currentDir "wsl_tools\gen_pack_wsl.sh"
if (Test-Path $genPackScript) {
    # Use WSL dos2unix or sed to fix line endings
    wsl bash -c "cd '$wslPath/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh"
    Write-Host "Line endings fixed." -ForegroundColor Green
}

Write-Host ""
Write-Host "Running gen_pack_wsl.sh in WSL..." -ForegroundColor Green
Write-Host ""

# Execute the script in WSL
wsl bash -c "cd '$wslPath' && bash wsl_tools/gen_pack_wsl.sh"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Pack generation completed successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    
    # Check if output directory exists
    if (Test-Path "output") {
        Write-Host "Contents of output directory:" -ForegroundColor Cyan
        Get-ChildItem "output" | ForEach-Object {
            Write-Host "  - $($_.Name)" -ForegroundColor White
        }
    }
} else {
    Write-Host ""
    Write-Host "ERROR: Pack generation failed!" -ForegroundColor Red
    Write-Host "Please check the error messages above." -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to exit"
