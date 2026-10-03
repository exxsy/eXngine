@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

SET "EXN_EXTRA=-DEXN_IMAGE_STRATEGY_STBI -D_GLFW_WIN32 -DVK_VERSION_1_0 -DGLFW_EXPOSE_NATIVE_WIN32 -DGLFW_INCLUDE_VULKAN -DNOMINMAX"

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
if /I "%CMD%"=="example" goto :do_example
if /I "%CMD%"=="renderer" goto :do_renderer
if /I "%CMD%"=="core" goto :do_core
if /I "%CMD%"=="physics" goto :do_physics
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

:do_physics
call :physics_build || goto :fail
goto :success

:do_window
call :window_build || goto :fail
goto :success

:do_assets
call :assets_build || goto :fail
goto :success

:do_example
call :example_build || goto :fail
goto :success

:do_renderer
call :renderer_build || goto :fail
goto :success

:do_clean
call :clean_output || goto :fail
goto :success

:do_all
call :build_shaders || goto :fail
call :core_build || goto :fail
call :window_build || goto :fail
call :assets_build || goto :fail
call :renderer_build || goto :fail
call :physics_build || goto :fail
call :example_build || goto :fail
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

:physics_build
powershell -Command "Write-Host '[physics] Checking for physics\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\physics\build.bat" (
	powershell -Command "Write-Host '[physics] Running physics\build.bat' -ForegroundColor Cyan"
	call "%CD%\src\physics\build.bat"
	if errorlevel 1 (
		echo [physics] Sub-build failed.
		exit /b 1
	)
) else (
	echo [physics] No physics\build.bat found. Skipping.
)
exit /b 0

:example_build
powershell -Command "Write-Host '[example] Checking for example\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\example\build.bat" (
	powershell -Command "Write-Host '[example] Running example\build.bat' -ForegroundColor Cyan"
	call "%CD%\src\example\build.bat"
	if errorlevel 1 (
		echo [example] Sub-build failed.
		exit /b 1
	)
) else (
	echo [example] No example\build.bat found. Skipping.
)
exit /b 0

:renderer_build
powershell -Command "Write-Host '[renderer] Checking for renderer\build.bat' -ForegroundColor Cyan"
if exist "%CD%\src\renderer\build.bat" (
	powershell -Command "Write-Host '[renderer] Running renderer\build.bat' -ForegroundColor Cyan"
	call "%CD%\src\renderer\build.bat"
	if errorlevel 1 (
		echo [renderer] Sub-build failed.
		exit /b 1
	)
) else (
	echo [renderer] No renderer\build.bat found. Skipping.
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
echo   example     Run example\build.bat (if present).
echo   renderer    Run renderer\build.bat (if present).
echo   physics     Run physics\build.bat (if present).
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

