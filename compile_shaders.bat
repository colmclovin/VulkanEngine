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
if not exist "Shaders" mkdir Shaders
REM Compile vertex shader
if exist "Shaders\ui_sprite.vert" (
	echo Compiling vertex shader...
	glslc Shaders\ui_sprite.vert -o Shaders\ui_sprite_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Vertex shader compiled successfully
	) else (
		echo   Vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: ui_sprite.vert not found
)
REM Compile fragment shader
if exist "Shaders\ui_sprite.frag" (
	echo Compiling fragment shader...
	glslc Shaders\ui_sprite.frag -o Shaders\ui_sprite_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Fragment shader compiled successfully
	) else (
		echo   Fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: ui_sprite.frag not found
)
REM Compile mesh vertex shader
if exist "Shaders\mesh.vert" (
	echo Compiling mesh vertex shader...
	glslc Shaders\mesh.vert -o Shaders\mesh_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Mesh vertex shader compiled successfully
	) else (
		echo   Mesh vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: mesh.vert not found
)
REM Compile mesh fragment shader
if exist "Shaders\mesh.frag" (
	echo Compiling mesh fragment shader...
	glslc Shaders\mesh.frag -o Shaders\mesh_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Mesh fragment shader compiled successfully
	) else (
		echo   Mesh fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: mesh.frag not found
)
echo.
echo Shader compilation complete!
dir shaders\*.spv /b
pause