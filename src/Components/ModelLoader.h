#pragma once
#include "Material.h"
#include <assimp/matrix4x4.h>
#include <string>
#include "../Renderer/MeshRenderer.h"
#include "AnimationClip.h"
#include "Skeleton.h"
#include "SkinnedMesh.h"

struct aiMesh;
class Mesh;
struct aiMaterial;
struct aiNode;
struct aiScene;

class ModelLoader {
public:
    static Mesh LoadModel(const std::string &filepath, VulkanEngine *engine, MeshRenderer *meshRenderer);
    static SkinnedMesh LoadSkinnedModel(const std::string &filepath, VulkanEngine *engine, MeshRenderer *meshRenderer);

private:

    static void ProcessNode(aiNode *node, const aiScene *scene, const aiMatrix4x4 &parentTransform, Mesh &outMesh);
    static void ProcessMesh(aiMesh *aiMeshData, const aiMatrix4x4 &transform, Mesh &outMesh);
    static Material ProcessMaterial(const aiScene *scene, aiMaterial *aiMat, VulkanEngine *engine, MeshRenderer *meshRenderer);
    static Skeleton ExtractSkeleton(const aiScene *scene, aiMesh *mesh);
    static std::vector<AnimationClip> ExtractAnimations(const aiScene *scene);
    static void ProcessSkinnedMesh(const aiScene *scene, aiMesh *aiMeshData, const aiMatrix4x4 &transform, SkinnedMesh &outMesh);
};