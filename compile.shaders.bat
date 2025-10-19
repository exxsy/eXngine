@echo off

@echo %1 %2

@REM %VULKAN_SDK%/Bin/glslangValidator.exe -V ./shaders/shader.frag --entry-point %1 -o output/%%~nxi.spv
@REM %VULKAN_SDK%/Bin/glslangValidator.exe -V ./shaders/shader.vert --entry-point %2 -o output/%%~nxi.spv

for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslangValidator.exe -V %%i -e %1 -e %2 -o output/%%~nxi.spv
@REM for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslc.exe %%i -e %1 -e %2 -o output/%%~nxi.spv

pause