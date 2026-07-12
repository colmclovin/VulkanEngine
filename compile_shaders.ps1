# Shader Compilation Script for Vulkan
# This script compiles GLSL shaders to SPIR-V bytecode

Write-Host "Compiling shaders..." -ForegroundColor Cyan

# Check if glslc is available
$glslc = Get-Command glslc -ErrorAction SilentlyContinue
if (-not $glslc) {
	Write-Host "ERROR: glslc not found! Make sure Vulkan SDK is installed and in PATH." -ForegroundColor Red
	exit 1
}

# Create output directory if it doesn't exist
$shaderDir = "Shaders"
if (-not (Test-Path $shaderDir)) {
	New-Item -ItemType Directory -Path $shaderDir | Out-Null
	Write-Host "Created Shaders directory" -ForegroundColor Green
}

# Compile vertex shader
if (Test-Path "$shaderDir\triangle.vert") {
	Write-Host "Compiling vertex shader..." -ForegroundColor Yellow
	glslc "$shaderDir\triangle.vert" -o "$shaderDir\vert.spv"
	if ($LASTEXITCODE -eq 0) {
		Write-Host "  ✓ Vertex shader compiled successfully" -ForegroundColor Green
	} else {
		Write-Host "  ✗ Vertex shader compilation failed" -ForegroundColor Red
		exit 1
	}
} else {
	Write-Host "WARNING: triangle.vert not found" -ForegroundColor Yellow
}

# Compile fragment shader
if (Test-Path "$shaderDir\triangle.frag") {
	Write-Host "Compiling fragment shader..." -ForegroundColor Yellow
	glslc "$shaderDir\triangle.frag" -o "$shaderDir\frag.spv"
	if ($LASTEXITCODE -eq 0) {
		Write-Host "  ✓ Fragment shader compiled successfully" -ForegroundColor Green
	} else {
		Write-Host "  ✗ Fragment shader compilation failed" -ForegroundColor Red
		exit 1
	}
} else {
	Write-Host "WARNING: triangle.frag not found" -ForegroundColor Yellow
}

Write-Host "`nShader compilation complete!" -ForegroundColor Cyan
Write-Host "Compiled files:" -ForegroundColor Cyan
Get-ChildItem "$shaderDir\*.spv" | ForEach-Object {
	Write-Host "  - $($_.Name)" -ForegroundColor Gray
}
