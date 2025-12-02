@echo off
chcp 65001 >nul
REM fc_embed Pack 本地生成工具 (WSL)
REM 双击运行即可生成 Pack 文件

echo ========================================
echo fc_embed Pack 本地生成工具 (WSL)
echo ========================================
echo.

REM 获取脚本所在目录的父目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."

REM 检查 WSL
wsl --status >nul 2>&1
if errorlevel 1 (
    echo 错误: WSL 未安装或无法访问！
    echo 请先安装 WSL: wsl --install
    echo.
    pause
    exit /b 1
)

REM 转换 Windows 路径到 WSL 路径
for %%i in ("%PROJECT_DIR%") do set "ABS_PATH=%%~fi"
set "DRIVE=%ABS_PATH:~0,1%"
set "REST_PATH=%ABS_PATH:~2%"
set "REST_PATH=%REST_PATH:\=/%"

REM 转换驱动器号为小写
set "DRIVE_LOWER="
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    if /i "%DRIVE%"=="%%i" set "DRIVE_LOWER=%%i"
)

set "WSL_PATH=/mnt/%DRIVE_LOWER%%REST_PATH%"

echo 工作目录: %ABS_PATH%
echo.
echo 正在生成 Pack...
echo.

REM 修复行尾并生成 Pack
wsl bash -c "cd '%WSL_PATH%/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh && chmod +x gen_pack_wsl.sh"
wsl bash -c "cd '%WSL_PATH%' && bash wsl_tools/gen_pack_wsl.sh"

if errorlevel 1 (
    echo.
    echo ========================================
    echo 生成失败！请检查错误信息。
    echo ========================================
) else (
    echo.
    echo ========================================
    echo 生成成功！
    echo ========================================
    echo.
    echo 输出文件位于 output 目录:
    if exist "%PROJECT_DIR%\output\*.pack" (
        dir /b "%PROJECT_DIR%\output\*.pack"
    )
)

echo.
pause
