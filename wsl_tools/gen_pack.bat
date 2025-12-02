@echo off
chcp 65001 >nul
REM fc_embed CMSIS Pack Generation Tool (WSL)
REM Double-click to generate Pack file

echo ==========================================
echo Simplified CMSIS Pack Generation
echo ==========================================
echo.

REM Get parent directory of script location
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."

REM Check WSL
wsl --status >nul 2>&1
if errorlevel 1 (
    echo Error: WSL is not installed or not accessible!
    echo Please install WSL first: wsl --install
    echo.
    pause
    exit /b 1
)

REM Convert Windows path to WSL path
for %%i in ("%PROJECT_DIR%") do set "ABS_PATH=%%~fi"
set "DRIVE=%ABS_PATH:~0,1%"
set "REST_PATH=%ABS_PATH:~2%"
set "REST_PATH=%REST_PATH:\=/%"

REM Convert drive letter to lowercase
set "DRIVE_LOWER="
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    if /i "%DRIVE%"=="%%i" set "DRIVE_LOWER=%%i"
)

set "WSL_PATH=/mnt/%DRIVE_LOWER%%REST_PATH%"

echo Working directory: %ABS_PATH%
echo.
echo Generating Pack...
echo.

REM Fix line endings and generate Pack
wsl bash -c "cd '%WSL_PATH%/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh && chmod +x gen_pack_wsl.sh"
wsl bash -c "cd '%WSL_PATH%' && bash wsl_tools/gen_pack_wsl.sh"

if errorlevel 1 (
    echo.
    echo ========================================
    echo Generation failed! Please check errors.
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Generation successful!
    echo ========================================
    echo.
    echo Output files are in the output directory:
    if exist "%PROJECT_DIR%\output\*.pack" (
        dir /b "%PROJECT_DIR%\output\*.pack"
    )
)

echo.
pause
