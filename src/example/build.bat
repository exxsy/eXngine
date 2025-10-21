@ECHO OFF
SetLocal EnableDelayedExpansion

IF /I "%~1"=="/?" GOTO :usage
IF /I "%~1"=="-?" GOTO :usage
IF /I "%~1"=="help" GOTO :usage

SET "CONFIG=Debug"
SET "ARCH=x64"
IF NOT "%~1"=="" SET "CONFIG=%~1"
IF NOT "%~2"=="" SET "ARCH=%~2"

REM Capture optional passthrough options for Clang after the first two args
SET "EXTRA_OPTS="
REM Shift off the first two args so %* now holds only extra options
SHIFT
SHIFT
IF NOT "%~1"=="" SET "EXTRA_OPTS=%*"
REM Also allow extras via environment variable
IF DEFINED EXN_EXTRA SET "EXTRA_OPTS=%EXTRA_OPTS% %EXN_EXTRA%"

IF /I "%ARCH%"=="x86" SET "ARCH=x32"
IF /I "%ARCH%"=="win32" SET "ARCH=x32"
IF /I NOT "%ARCH%"=="x64" IF /I NOT "%ARCH%"=="x32" (
    powershell -Command "Write-Host '[WARN] Unknown ARCH \"%ARCH%\". Falling back to x64.' -ForegroundColor Yellow"
    SET "ARCH=x64"
)

SET "cFilenames="
FOR /R "src\example\src" %%f IN (*.c) DO (
    SET "cFilenames=!cFilenames! %%f"
)
FOR /R "src\example\src" %%f IN (*.cpp) DO (
    SET "cFilenames=!cFilenames! %%f"   
)

powershell -Command "Write-Host 'Files: %cFilenames%' -ForegroundColor Cyan"

SET "namespace=eXngine"
SET "assembly=example"
SET "compilerFlags=-std=c++20 -Werror -Wno-error=deprecated-builtins -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=nontrivial-memcall -Wno-error=reorder-ctor"

IF /I "%ARCH%"=="x64" (
    SET "compilerFlags=%compilerFlags% -m64"
) ELSE (
    SET "compilerFlags=%compilerFlags% -m32"
)

IF /I "%CONFIG%"=="Debug" (
    SET "compilerFlags=%compilerFlags% -g -O0"
    SET "defines=-D_DEBUG"
    SET "linkerFlags=%linkerFlags% -lmsvcrtd -lvulkan-1"
) ELSE (
    SET "CONFIG=Release"
    SET "compilerFlags=%compilerFlags% -O2"
    SET "defines=-DNDEBUG"
    SET "linkerFlags=%linkerFlags% -lmsvcrt -lvulkan-1"
)

SET "THIRDPARTY_LIB_DIR=%CD%\3rdparty\lib"
SET "includeFlags=-Isrc -Iincludes -I3rdparty -I3rdparty\imgui -I3rdparty\glm -I3rdparty\glfw\include -II3rdparty\glfw\src -I3rdparty\fbx"
SET "includeFlags=%includeFlags% -I%VULKAN_SDK%\Include"
SET "linkerFlags=%linkerFlags% -L%THIRDPARTY_LIB_DIR% -L%THIRDPARTY_LIB_DIR%\%ARCH%\%CONFIG% -L%CD%\output\%ARCH%\%CONFIG%"
SET "linkerFlags=%linkerFlags% -L%VULKAN_SDK%\Lib -lvulkan-1 -llibfbxsdk -lglfw3"
SET "linkerFlags=%linkerFlags% -leXngine.renderer -leXngine.core -leXngine.window -leXngine.assets -lshell32 -lgdi32 -luser32"

REM Append Windows subsystem and entry point without clobbering previous flags
SET "linkerFlags=%linkerFlags% -Xlinker /SUBSYSTEM:WINDOWS -Xlinker /ENTRY:WinMainCRTStartup -Xlinker /NODEFAULTLIB:libcmt"
SET "OUT_DIR=%CD%\output\%ARCH%\%CONFIG%"

IF NOT EXIST "%OUT_DIR%" (
    MKDIR "%OUT_DIR%" >NUL 2>&1
)

powershell -Command "Write-Host 'Building %assembly% for %ARCH% %CONFIG%...' -ForegroundColor Cyan"

clang++ %cFilenames% %compilerFlags% %defines% %EXTRA_OPTS% %includeFlags% %linkerFlags% -o "%OUT_DIR%\%namespace%.%assembly%.exe"

SET "ERR=%ERRORLEVEL%"

IF NOT "%ERR%"=="0" (
    powershell -Command "Write-Host 'Build failed with ERROR_LEVEL=%ERR%' -ForegroundColor Red"
    EXIT /B %ERR%
)

powershell -Command "Write-Host 'Build succeeded: %OUT_DIR%\%namespace%.%assembly%.exe' -ForegroundColor Green"
EXIT /B 0

:usage
ECHO Usage: build.bat [Config] [Arch]
ECHO   Config: Debug ^| Release   (default: Debug)
ECHO   Arch:   x64   ^| x32       (default: x64)
ECHO Examples:
ECHO   build.bat
ECHO   build.bat Release
ECHO   build.bat Debug x32
ECHO   build.bat Release x64 -DEXN_FOO=1 -Wno-unused-parameter
ECHO You can pass extra Clang options after the two args; they are forwarded as-is.
ECHO Alternatively set EXN_EXTRA environment variable to inject flags.
EXIT /B 0