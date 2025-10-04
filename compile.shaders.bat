@echo off

@REM %VULKAN_SDK%/Bin/glslc.exe shaders/shader.vert -o output/vert.spv
@REM %VULKAN_SDK%/Bin/glslc.exe shaders/shader.frag -o output/frag.spv

@REM for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslangValidator.exe -V %%i

for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslc.exe %%i

pause