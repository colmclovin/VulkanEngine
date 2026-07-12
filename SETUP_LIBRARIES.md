# Library Setup for Factorio Engine

This project requires the following libraries:

## Required Libraries

1. **EnTT** - Entity Component System (header-only)
2. **GLM** - OpenGL Mathematics (header-only)
3. **stb_image** - Image loading (header-only)

## Installation Options

### Option 1: vcpkg (Recommended)

```powershell
# Install vcpkg if you haven't already
# Then run:
vcpkg install entt:x64-windows
vcpkg install glm:x64-windows
vcpkg install stb:x64-windows

# Integrate with Visual Studio
vcpkg integrate install
```

### Option 2: Manual Installation

1. **EnTT**
   - Download from: https://github.com/skypjack/entt/releases
   - Extract `single_include/entt/entt.hpp` to `vendor/entt/entt.hpp`

2. **GLM**
   - Download from: https://github.com/g-truc/glm/releases
   - Extract `glm/` folder to `vendor/glm/`

3. **stb_image**
   - Download from: https://github.com/nothings/stb
   - Copy `stb_image.h` to `vendor/stb/stb_image.h`

4. **Add to Visual Studio**
   - Right-click project → Properties → C/C++ → General
   - Add to "Additional Include Directories": `$(ProjectDir)vendor`

## Verify Installation

After installation, the following includes should work:
```cpp
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

## Note
If using vcpkg, Visual Studio will automatically find the libraries after running `vcpkg integrate install`.
