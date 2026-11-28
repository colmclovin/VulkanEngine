@echo off
REM Compiles every shader in Shaders\ to SPIR-V:  name.vert -> name_vert.spv, name.comp -> name_comp.spv, etc.
REM Building in Visual Studio already does this (CompileShaders target in VulkanEngine.vcxproj).
REM Use this to recompile shaders without building the C++ code.  Pass "nopause" to skip the pause at the end.
setlocal
cd /d "%~dp0"

set "GLSLC=glslc.exe"
if defined VULKAN_SDK set "GLSLC=%VULKAN_SDK%\Bin\glslc.exe"

set FAILED=0
for %%S in (vert frag comp) do (
    for %%F in (Shaders\*.%%S) do (
        "%GLSLC%" -I Shaders "%%F" -o "Shaders\%%~nF_%%S.spv"
        if errorlevel 1 (
            echo FAILED: %%F
            set FAILED=1
        ) else (
            echo   %%~nxF
        )
    )
)

if %FAILED%==1 (echo Some shaders failed to compile.) else (echo All shaders compiled.)
if /i not "%~1"=="nopause" pause
exit /b %FAILED%
