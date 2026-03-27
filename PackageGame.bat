@echo off
REM ============================================================================
REM  PackageGame.bat — Package Asteroid Survivor into a standalone executable
REM
REM  Usage:
REM    PackageGame.bat                          (auto-detect engine path)
REM    PackageGame.bat "C:\Epic Games\UE_5.7"  (explicit engine path)
REM
REM  The packaged game is written to Build\WindowsPackage by default.
REM ============================================================================
setlocal enabledelayedexpansion

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%AsteroidSurvivor.uproject"
set "OUTPUT_DIR=%PROJECT_DIR%Build\WindowsPackage"
set "PLATFORM=Win64"
set "CONFIG=Shipping"

REM --- Resolve engine root ------------------------------------------------
if not "%~1"=="" (
    set "UE_ROOT=%~1"
    goto :check_engine
)

REM Try common Epic Games Launcher install paths
for %%P in (
    "C:\Program Files\Epic Games\UE_5.7"
    "C:\Program Files (x86)\Epic Games\UE_5.7"
    "D:\Epic Games\UE_5.7"
    "E:\Epic Games\UE_5.7"
    "C:\Program Files\Epic Games\UE_5.6"
    "C:\Program Files\Epic Games\UE_5.5"
    "C:\Program Files\Epic Games\UE_5.4"
) do (
    if exist "%%~P\Engine\Build\BatchFiles\RunUAT.bat" (
        set "UE_ROOT=%%~P"
        goto :check_engine
    )
)

REM Check the UE_ROOT environment variable
if defined UE_ROOT (
    if exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" goto :check_engine
)

echo.
echo ERROR: Could not find Unreal Engine installation.
echo.
echo Please either:
echo   1. Pass the engine path as an argument:
echo      PackageGame.bat "C:\Program Files\Epic Games\UE_5.7"
echo   2. Set the UE_ROOT environment variable:
echo      set UE_ROOT=C:\Program Files\Epic Games\UE_5.7
echo.
exit /b 1

:check_engine
set "RUNUAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%RUNUAT%" (
    echo ERROR: RunUAT.bat not found at "%RUNUAT%"
    echo        Check that your engine path is correct.
    exit /b 1
)

echo.
echo ============================================================
echo   Packaging Asteroid Survivor
echo ============================================================
echo   Engine : %UE_ROOT%
echo   Project: %UPROJECT%
echo   Output : %OUTPUT_DIR%
echo   Config : %CONFIG%
echo ============================================================
echo.

call "%RUNUAT%" BuildCookRun ^
    -project="%UPROJECT%" ^
    -noP4 ^
    -platform=%PLATFORM% ^
    -clientconfig=%CONFIG% ^
    -serverconfig=%CONFIG% ^
    -cook ^
    -map=/Game/Maps/GameLevel+/Game/Maps/MainMenu ^
    -build ^
    -stage ^
    -pak ^
    -archive ^
    -archivedirectory="%OUTPUT_DIR%" ^
    -utf8output

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Packaging failed. Check the log output above for details.
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================================
echo   SUCCESS! Packaged game is in:
echo   %OUTPUT_DIR%
echo.
echo   Share the entire folder with your friend.
echo   They can run AsteroidSurvivor.exe to play!
echo ============================================================
echo.

endlocal
