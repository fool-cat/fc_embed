# Local Pack Generation using WSL
# 本地使用 WSL 生成 CMSIS Pack 的一键脚本

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "fc_embed Pack 本地生成工具 (WSL)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查 WSL
try {
    $null = wsl --status 2>&1
    if ($LASTEXITCODE -ne 0) { throw }
} catch {
    Write-Host "错误: WSL 未安装或无法访问！" -ForegroundColor Red
    Write-Host "请先安装 WSL: wsl --install" -ForegroundColor Yellow
    Read-Host "按 Enter 退出"
    exit 1
}

# 转换路径
$winPath = Split-Path -Parent $PSScriptRoot
$driveLetter = $winPath.Substring(0, 1).ToLower()
$restOfPath = $winPath.Substring(2) -replace '\\', '/'
$wslPath = "/mnt/$driveLetter$restOfPath"

Write-Host "工作目录: $winPath" -ForegroundColor Yellow
Write-Host ""

# 修复行尾并生成 Pack
Write-Host "正在生成 Pack..." -ForegroundColor Green
wsl bash -c "cd '$wslPath/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh && chmod +x gen_pack_wsl.sh"
wsl bash -c "cd '$wslPath' && bash wsl_tools/gen_pack_wsl.sh"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "生成成功！" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    
    if (Test-Path "$winPath\output") {
        Write-Host "输出文件:" -ForegroundColor Cyan
        Get-ChildItem "$winPath\output\*.pack" | ForEach-Object {
            $size = [math]::Round($_.Length / 1KB, 1)
            Write-Host "  $($_.Name) ($size KB)" -ForegroundColor White
        }
    }
} else {
    Write-Host ""
    Write-Host "生成失败！请检查错误信息。" -ForegroundColor Red
}

Write-Host ""
Read-Host "按 Enter 退出"
