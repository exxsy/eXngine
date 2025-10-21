@echo off
FOR /r %%i IN (shaders\*.frag, shaders\*.vert) DO (
    %VULKAN_SDK%/Bin/glslangValidator.exe -V %%i -o output/%%~nxi.spv
    IF ERRORLEVEL 1 (
        powershell -Command "Write-Host 'Shader compilation failed' -ForegroundColor Red"
        exit /b 1
    )
)
powershell -Command "Write-Host 'Shader compilation succeeded' -ForegroundColor Green"
@REM for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslc.exe %%i -e %1 -e %2 -o output/%%~nxi.spv