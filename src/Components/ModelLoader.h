#pragma once
#include "Material.h"
#include <assimp/matrix4x4.h>
#include <string>

struct aiMesh;
class Mesh;
struct aiMaterial;
struct aiNode;
struct aiScene;

class ModelLoader {
public:
    static Mesh LoadModel(const std::string &filepath);

private:
    static void ProcessNode(aiNode *node, const aiScene *scene, const aiMatrix4x4 &parentTransform, Mesh &outMesh);
    static void ProcessMesh(aiMesh *aiMeshData, const aiMatrix4x4 &transform, Mesh &outMesh);
    static Material ProcessMaterial(aiMaterial *aiMat);
};