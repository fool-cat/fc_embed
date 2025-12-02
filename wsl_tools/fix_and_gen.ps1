# Quick fix for line endings and generate pack
# Run this once to fix the issue

$winPath = (Get-Location).Path
$driveLetter = $winPath.Substring(0, 1).ToLower()
$restOfPath = $winPath.Substring(2) -replace '\\', '/'
$wslPath = "/mnt/$driveLetter$restOfPath"

Write-Host "Fixing line endings in gen_pack_wsl.sh..." -ForegroundColor Yellow
wsl bash -c "cd '$wslPath/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh && chmod +x gen_pack_wsl.sh"
Write-Host "Fixed!" -ForegroundColor Green
Write-Host ""
Write-Host "Now generating pack..." -ForegroundColor Cyan
wsl bash -c "cd '$wslPath' && bash wsl_tools/gen_pack_wsl.sh"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Success!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Failed!" -ForegroundColor Red
}
