@echo off
REM Windows batch file to generate CMSIS Pack using WSL
REM Author: FoolCat
REM Date: 2025-12-02

echo ========================================
echo Generating CMSIS Pack using WSL
echo ========================================
echo.

REM Get current directory in Windows format (parent of wsl_tools)
set "WIN_PATH=%~dp0.."

REM Convert Windows path to WSL path
REM Remove trailing backslash
if "%WIN_PATH:~-1%"=="\" set "WIN_PATH=%WIN_PATH:~0,-1%"

REM Convert to WSL format (e.g., D:\GitHub_Clone\fc_embed -> /mnt/d/GitHub_Clone/fc_embed)
for /f "tokens=1,* delims=:" %%a in ("%WIN_PATH%") do (
    set "DRIVE=%%a"
    set "REST=%%b"
)

REM Convert drive letter to lowercase
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    call set "DRIVE=%%DRIVE:%%i=%%i%%"
)

REM Replace backslashes with forward slashes
set "REST=%REST:\=/%"

REM Build WSL path
set "WSL_PATH=/mnt/%DRIVE:~0,1%%REST%"

echo Windows Path: %WIN_PATH%
echo WSL Path: %WSL_PATH%
echo.

REM Check if WSL is installed
wsl --status >nul 2>&1
if errorlevel 1 (
    echo ERROR: WSL is not installed or not accessible!
    echo Please install WSL first using: wsl --install
    pause
    exit /b 1
)

echo Running gen_pack_wsl.sh in WSL...
echo.

REM Fix line endings first (CRLF to LF)
echo Fixing line endings...
wsl bash -c "cd '%WSL_PATH%/wsl_tools' && sed -i 's/\r$//' gen_pack_wsl.sh"
echo Line endings fixed.
echo.

REM Execute the script in WSL
wsl cd "%WSL_PATH%" ^&^& bash wsl_tools/gen_pack_wsl.sh

if errorlevel 1 (
    echo.
    echo ERROR: Pack generation failed!
    echo Please check the error messages above.
) else (
    echo.
    echo ========================================
    echo Pack generation completed successfully!
    echo ========================================
    echo.
    echo The generated pack file should be in the 'output' directory.
    if exist "output" (
        echo.
        echo Contents of output directory:
        dir /b output
    )
)

echo.
pause
