#pragma once
#include <memory>
#include "Mesh.h"

struct MeshComponent {
    // std::string meshFilePath;
    // std::string textureFilePath;
    std::shared_ptr<Mesh> mesh;
};
