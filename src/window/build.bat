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
    ECHO [WARN] Unknown ARCH "%ARCH%". Falling back to x64.
    SET "ARCH=x64"
)

SET "cFilenames="
FOR /R %%f IN (src\window\*.c) DO (
    SET "cFilenames=!cFilenames! %%f"
)
FOR /R %%f IN (src\window\*.cpp) DO (
    SET "cFilenames=!cFilenames! %%f"
)

ECHO Files: %cFilenames%

SET "namespace=eXngine"
SET "assembly=window"
SET "compilerFlags=-std=c++20 -shared -Wvarargs -Wall -Werror"

IF /I "%ARCH%"=="x64" (
    SET "compilerFlags=%compilerFlags% -m64"
) ELSE (
    SET "compilerFlags=%compilerFlags% -m32"
)

IF /I "%CONFIG%"=="Debug" (
    SET "compilerFlags=%compilerFlags% -g -O0"
    SET "defines=-D_DEBUG -DEXNEXPORT"
    SET "linkerFlags=%linkerFlags% -lmsvcrtd"
) ELSE (
    SET "CONFIG=Release"
    SET "compilerFlags=%compilerFlags% -O2"
    SET "defines=-DNDEBUG -DEXNEXPORT"
    SET "linkerFlags=%linkerFlags% -lmsvcrt"
)

SET "THIRDPARTY_LIB_DIR==%CD%\3rdparty\lib\%ARCH%\%CONFIG%"
SET "includeFlags=-Isrc -I..\includes -I..\3rdparty\glfw\include -I..\3rdparty\glfw\src"
SET "linkerFlags=%linkerFlags% -L%THIRDPARTY_LIB_DIR% -lglfw3 -lshell32 -lgdi32 -luser32"
SET "OUT_DIR=%CD%\output\%ARCH%\%CONFIG%"
IF NOT EXIST "%OUT_DIR%" (
    MKDIR "%OUT_DIR%" >NUL 2>&1
)

ECHO Building %assembly% for %ARCH% %CONFIG%...

clang %cFilenames% %compilerFlags% %defines% %includeFlags% %EXTRA_OPTS% %linkerFlags% -o "%OUT_DIR%\%namespace%.%assembly%.dll"

SET "ERR=%ERRORLEVEL%"

IF NOT "%ERR%"=="0" (
    ECHO Build failed with ERROR_LEVEL=%ERR%
    EXIT /B %ERR%
)

ECHO Build succeeded: %OUT_DIR%\eXngine.%assembly%.dll
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