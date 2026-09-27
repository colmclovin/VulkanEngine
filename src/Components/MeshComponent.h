#pragma once
#include <memory>
#include "../Rendering/Mesh.h"

struct MeshComponent {
    // std::string meshFilePath;
    // std::string textureFilePath;
    std::shared_ptr<Mesh> mesh;
};
