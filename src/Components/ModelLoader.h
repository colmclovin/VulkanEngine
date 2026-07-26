#pragma once
#include <string>

#include "Material.h"
// Forward declarations to avoid including Assimp in header
struct aiMesh;
class Mesh;
struct aiMaterial;

class ModelLoader {
public:
    static Mesh LoadModel(const std::string &filepath);
    

private:
    static void ProcessMesh(aiMesh *aiMeshData, Mesh &mesh);
    static Material ProcessMaterial(aiMaterial* aiMat);
};