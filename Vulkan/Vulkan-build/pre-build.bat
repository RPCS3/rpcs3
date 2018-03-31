@ECHO OFF

REM Setting up dummy git helper
MKDIR ..\Vulkan-LoaderAndValidationLayers\external\glslang\External\spirv-tools
SET header="@ECHO off"
SET content="ECHO 12345678deadbeef12345678cafebabe12345678"
ECHO %header:"=% > git.bat
ECHO %content:"=% >> git.bat
COPY /Y git.bat ..\Vulkan-LoaderAndValidationLayers\external\glslang\External\spirv-tools\git.bat

REM Set up gitignore
SET ignored="*"
ECHO %ignored:"=% > ..\Vulkan-LoaderAndValidationLayers\external\glslang\External\spirv-tools\.gitignore