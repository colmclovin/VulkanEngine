@echo off
REM Shader compilation batch script for Windows

echo Compiling shaders...

REM Check if glslc exists
where glslc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
	echo ERROR: glslc not found! Make sure Vulkan SDK is installed and in PATH.
	exit /b 1
)

REM Create shaders directory if it doesn't exist
if not exist "shaders" mkdir shaders

REM Compile vertex shader
if exist "shaders\triangle.vert" (
	echo Compiling vertex shader...
	glslc shaders\triangle.vert -o shaders\vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Vertex shader compiled successfully
	) else (
		echo   Vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: triangle.vert not found
)

REM Compile fragment shader
if exist "shaders\triangle.frag" (
	echo Compiling fragment shader...
	glslc shaders\triangle.frag -o shaders\frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Fragment shader compiled successfully
	) else (
		echo   Fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: triangle.frag not found
)

echo.
echo Shader compilation complete!
dir shaders\*.spv /b

pause
