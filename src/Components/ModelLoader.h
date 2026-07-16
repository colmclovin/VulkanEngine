#pragma once
#include <string>

// Forward declarations to avoid including Assimp in header
struct aiMesh;
class Mesh;

class ModelLoader {
public:
    static Mesh LoadModel(const std::string &filepath);

private:
    static void ProcessMesh(aiMesh *aiMeshData, Mesh &mesh);
};