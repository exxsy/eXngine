@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

rem Move to repository root (folder of this script)
pushd "%~dp0"

rem ---- Defaults (override via env vars before calling this script) ----
if not defined BUILD_DIR set "BUILD_DIR=%CD%\output"

set "EXIT_CODE=0"

set "CMD=%~1"
if "%CMD%"=="" set "CMD=all"

if /I "%CMD%"=="help" goto :usage
if /I "%CMD%"=="shaders" goto :do_shaders
if /I "%CMD%"=="window" goto :do_window
if /I "%CMD%"=="assets" goto :do_assets
if /I "%CMD%"=="clean" goto :do_clean
if /I "%CMD%"=="all" goto :do_all

echo Unknown command: %CMD%
echo.
goto :usage

rem --------------------------- Commands ---------------------------

:do_shaders
call :build_shaders || goto :fail
goto :success

:do_core
call :core_build || goto :fail
goto :success

:do_window
call :window_build || goto :fail
goto :success

:do_assets
call :assets_build || goto :fail
goto :success

:do_clean
call :clean_output || goto :fail
goto :success

:do_all
call :build_shaders || goto :fail
call :core_build || goto :fail
call :assets_build || goto :fail
call :window_build || goto :fail
goto :success

rem ------------------------- Subroutines --------------------------

:build_shaders
powershell -Command "Write-Host '[shaders] Checking for build.shaders.bat' -ForegroundColor Cyan"
if exist "%CD%\build.shaders.bat" (
	powershell -Command "Write-Host '[shaders] Running build.shaders.bat' -ForegroundColor Cyan"
	call "%CD%\build.shaders.bat"
	if errorlevel 1 (
		echo [shaders] Failed.
		exit /b 1
	)
) else (
	echo [shaders] No build.shaders.bat found. Skipping.
)
exit /b 0

:window_build
powershell -Command "Write-Host '[window] Checking for window\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\window\build.bat" (
	powershell -Command "Write-Host '[window] Running window\build.bat' -ForegroundColor Cyan"
	call "%CD%\src\window\build.bat"
	if errorlevel 1 (
		echo [window] Sub-build failed.
		exit /b 1
	)
) else (
	echo [window] No window\build.bat found. Skipping.
)
exit /b 0

:assets_build
powershell -Command "Write-Host '[assets] Checking for assets\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\assets\build.bat" (
	powershell -Command "Write-Host '[assets] Running assets\build.bat' -ForegroundColor Cyan"
	SET "EXN_EXTRA=-DEXN_TEXTURE_STRATEGY_STBI"
	call "%CD%\src\assets\build.bat"
	if errorlevel 1 (
		echo [assets] Sub-build failed.
		exit /b 1
	)
) else (
	echo [assets] No assets\build.bat found. Skipping.
)
exit /b 0

:core_build
powershell -Command "Write-Host '[core] Checking for core\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\core\build.bat" (
	powershell -Command "Write-Host '[core] Running core\build.bat' -ForegroundColor Cyan"
	call "%CD%\src\core\build.bat"
	if errorlevel 1 (
		echo [core] Sub-build failed.
		exit /b 1
	)
) else (
	echo [core] No core\build.bat found. Skipping.
)
exit /b 0

:clean_output
powershell -Command "Write-Host '[clean] Deleting output directory \"%BUILD_DIR%\" (if present)' -ForegroundColor Cyan"
if exist "%BUILD_DIR%" (
	rmdir /S /Q "%BUILD_DIR%"
	if errorlevel 1 (
		echo [clean] Failed to remove output directory.
		exit /b 1
	)
)
exit /b 0

rem ---------------------------- Meta ------------------------------

:usage
echo Usage: build [command]
echo.
echo Commands:
echo   help        Show this help.
echo   shaders     Compile shaders via build.shaders.bat (if present).
echo   window      Run window\build.bat (if present).
echo   assets      Run assets\build.bat (if present).
echo   core        Run core\build.bat (if present).
echo   clean       Delete the output directory.
echo   all         shaders + window  ^(default^)
echo.
echo Environment overrides:
echo   BUILD_DIR   Output folder  ^(default: ^"%CD%\output^"^)
echo.
goto :teardown

:success
echo.
powershell -Command "Write-Host '[ok] Done.' -ForegroundColor Green"
set "EXIT_CODE=0"
goto :teardown

:fail
echo.
powershell -Command "Write-Host '[error] Build script failed.' -ForegroundColor Red"
set "EXIT_CODE=1"
goto :teardown

:teardown
popd
endlocal & exit /b %EXIT_CODE%

