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
REM Compile debug line vertex shader
if exist "Shaders\debugline.vert" (
	echo Compiling debug line vertex shader...
	glslc Shaders\debugline.vert -o Shaders\debugline_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Debug line vertex shader compiled successfully
	) else (
		echo   Debug line vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: debugline.vert not found
)
REM Compile debug line fragment shader
if exist "Shaders\debugline.frag" (
	echo Compiling debug line fragment shader...
	glslc Shaders\debugline.frag -o Shaders\debugline_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Debug line fragment shader compiled successfully
	) else (
		echo   Debug line fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: debugline.frag not found
)
REM Compile skinned mesh vertex shader
if exist "Shaders\skinned.vert" (
	echo Compiling skinned mesh vertex shader...
	glslc Shaders\skinned.vert -o Shaders\skinned_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Skinned mesh vertex shader compiled successfully
	) else (
		echo   Skinned mesh vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: skinned.vert not found
)
REM Compile skinned mesh fragment shader
if exist "Shaders\skinned.frag" (
	echo Compiling skinned mesh fragment shader...
	glslc Shaders\skinned.frag -o Shaders\skinned_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Skinned mesh fragment shader compiled successfully
	) else (
		echo   Skinned mesh fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: skinned.frag not found
)

REM Compile shadow vertex shader
if exist "Shaders\shadow.vert" (
	echo Compiling shadow vertex shader...
	glslc Shaders\shadow.vert -o Shaders\shadow_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Shadow vertex shader compiled successfully
	) else (
		echo   Shadow vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: shadow.vert not found
)
REM Compile Shadow fragment shader
if exist "Shaders\shadow.frag" (
	echo Compiling shadow fragment shader...
	glslc Shaders\shadow.frag -o Shaders\shadow_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Shadow fragment shader compiled successfully
	) else (
		echo   Shadow fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: shadow.frag not found
)
REM Compile shadow skinned vertex shader
if exist "Shaders\shadow_skinned.vert" (
	echo Compiling shadow skinned vertex shader...
	glslc Shaders\shadow_skinned.vert -o Shaders\shadow_skinned_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Shadow skinned vertex shader compiled successfully
	) else (
		echo   Shadow skinned vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: shadow_skinned.vert not found
)
REM Compile shadow skinned fragment shader
if exist "Shaders\shadow_skinned.frag" (
	echo Compiling shadow skinned fragment shader...
	glslc Shaders\shadow_skinned.frag -o Shaders\shadow_skinned_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Shadow skinned fragment shader compiled successfully
	) else (
		echo   Shadow skinned fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: shadow_skinned.frag not found
)
REM Compile shadow skinned vertex shader
if exist "Shaders\mesh_instanced.vert" (
	echo Compiling mesh instanced vertex shader...
	glslc Shaders\mesh_instanced.vert -o Shaders\mesh_instanced_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Mesh Instanced vertex shader compiled successfully
	) else (
		echo   Mesh Instanced vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: mesh_instanced.vert not found
)
REM Compile mesh instanced fragment shader
if exist "Shaders\mesh_instanced.frag" (
	echo Compiling mesh instanced fragment shader...
	glslc Shaders\mesh_instanced.frag -o Shaders\mesh_instanced_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Mesh Instanced fragment shader compiled successfully
	) else (
		echo   Mesh Instanced fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: mesh_instanced.frag not found
)
REM Compile shadow instanced vertex shader
if exist "Shaders\shadow_instanced.vert" (
	echo Compiling shadow instanced vertex shader...
	glslc Shaders\shadow_instanced.vert -o Shaders\shadow_instanced_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Shadow Instanced vertex shader compiled successfully
	) else (
		echo   Shadow Instanced vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: shadow_instanced.vert not found
)
REM Compile chunk culling compute shader
if exist "Shaders\chunk_cull.comp" (
	echo Compiling chunk culling compute shader...
	glslc Shaders\chunk_cull.comp -o Shaders\chunk_cull_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Chunk culling compute shader compiled successfully
	) else (
		echo   Chunk culling compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: chunk_cull.comp not found
)
REM Compile terrain vertex shader
if exist "Shaders\terrain.vert" (
	echo Compiling terrain vertex shader...
	glslc Shaders\terrain.vert -o Shaders\terrain_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Terrain vertex shader compiled successfully
	) else (
		echo   Terrain vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: terrain.vert not found
)
REM Compile terrain fragment shader
if exist "Shaders\terrain.frag" (
	echo Compiling terrain fragment shader...
	glslc Shaders\terrain.frag -o Shaders\terrain_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Terrain fragment shader compiled successfully
	) else (
		echo   Terrain fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: terrain.frag not found
)

REM Compile noise test compute shader
if exist "Shaders\noise_test.comp" (
	echo Compiling noise test compute shader...
	glslc -I Shaders Shaders\noise_test.comp -o Shaders\noise_test_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Noise test compute shader compiled successfully
	) else (
		echo   Noise test compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: noise_test.comp not found
)

REM Compile noise test compute shader
if exist "Shaders\biome_test.comp" (
	echo Compiling biome test compute shader...
	glslc -I Shaders Shaders\biome_test.comp -o Shaders\biome_test_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   biome test compute shader compiled successfully
	) else (
		echo   biome test compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: biome_test.comp not found
)
if exist "Shaders\biome_grid_test.comp" (
	echo Compiling biome grid test compute shader...
	glslc -I Shaders Shaders\biome_grid_test.comp -o Shaders\biome_grid_test_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Biome grid test compute shader compiled successfully
	) else (
		echo   Biome grid test compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: biome_grid_test.comp not found
)
REM Compile chunk generation pass 1 compute shader
if exist "Shaders\chunk_generate_pass1.comp" (
	echo Compiling chunk generation pass 1 compute shader...
	glslc -I Shaders Shaders\chunk_generate_pass1.comp -o Shaders\chunk_generate_pass1_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Chunk generation pass 1 compute shader compiled successfully
	) else (
		echo   Chunk generation pass 1 compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: chunk_generate_pass1.comp not found
)
REM Compile chunk generation pass 2 compute shader
if exist "Shaders\chunk_generate_pass2.comp" (
	echo Compiling chunk generation pass 2 compute shader...
	glslc -I Shaders Shaders\chunk_generate_pass2.comp -o Shaders\chunk_generate_pass2_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Chunk generation pass 2 compute shader compiled successfully
	) else (
		echo   Chunk generation pass 2 compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: chunk_generate_pass2.comp not found
)
REM Compile tree culling compute shader
if exist "Shaders\tree_cull.comp" (
	echo Compiling tree culling compute shader...
	glslc -I Shaders Shaders\tree_cull.comp -o Shaders\tree_cull_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Tree culling compute shader compiled successfully
	) else (
		echo   Tree culling compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: tree_cull.comp not found
)
REM Compile tree generation compute shader
if exist "Shaders\tree_generate.comp" (
	echo Compiling tree generation compute shader...
	glslc -I Shaders Shaders\tree_generate.comp -o Shaders\tree_generate_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Tree generation compute shader compiled successfully
	) else (
		echo   Tree generation compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: tree_generate.comp not found
)
REM Compile tree instanced vertex shader
if exist "Shaders\tree_instanced.vert" (
	echo Compiling tree instanced vertex shader...
	glslc -I Shaders Shaders\tree_instanced.vert -o Shaders\tree_instanced_vert.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Tree instanced vertex shader compiled successfully
	) else (
		echo   Tree instanced vertex shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: tree_instanced.vert not found
)
REM Compile tree instanced fragment shader
if exist "Shaders\tree_instanced.frag" (
	echo Compiling tree instanced fragment shader...
	glslc -I Shaders Shaders\tree_instanced.frag -o Shaders\tree_instanced_frag.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Tree instanced fragment shader compiled successfully
	) else (
		echo   Tree instanced fragment shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: tree_instanced.frag not found
)

if exist "Shaders\simplex_debug.comp" (
	echo Compiling simplex debug compute shader...
	glslc -I Shaders Shaders\simplex_debug.comp -o Shaders\simplex_debug_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Simplex debug compute shader compiled successfully
	) else (
		echo   Simplex debug compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: simplex_debug.comp not found
)
if exist "Shaders\ridged_debug.comp" (
	echo Compiling ridged debug compute shader...
	glslc -I Shaders Shaders\ridged_debug.comp -o Shaders\ridged_debug_comp.spv
	if %ERRORLEVEL% EQU 0 (
		echo   Ridged debug compute shader compiled successfully
	) else (
		echo   Ridged debug compute shader compilation failed
		exit /b 1
	)
) else (
	echo WARNING: ridged_debug.comp not found
)


echo.
echo Shader compilation complete!
dir shaders\*.spv /b
if "%1"=="" pause