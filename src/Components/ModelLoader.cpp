// Prevent Windows.h macros from breaking Assimp
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "ModelLoader.h"
#include "Mesh.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <iostream>
#include <string>
#include <assimp/material.h>
#include "TextureLoader.h"

// ModelLoader.cpp
Mesh ModelLoader::LoadModel(const std::string &filepath, VulkanEngine *engine, MeshRenderer *meshRenderer) {
    Mesh mesh;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::string error = "Failed to load model: " + std::string(importer.GetErrorString());
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    std::cout << "Loaded model: " << filepath << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Materials: " << scene->mNumMaterials << std::endl;

   for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        mesh.Materials.push_back(ProcessMaterial(scene, scene->mMaterials[i], engine, meshRenderer));
    }

    // Walk the scene graph from the root, accumulating each node's transform
    ProcessNode(scene->mRootNode, scene, aiMatrix4x4(), mesh);   // identity at root

    std::cout << "  Vertices: " << mesh.Vertices.size() << std::endl;
    std::cout << "  Indices: " << mesh.Indices.size() << std::endl;
    std::cout << "  SubMeshes: " << mesh.SubMeshes.size() << std::endl;

    return mesh;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform, Mesh& outMesh) {
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, nodeTransform, outMesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, nodeTransform, outMesh);
    }
}

void ModelLoader::ProcessMesh(aiMesh* aiMeshData, const aiMatrix4x4& transform, Mesh& outMesh) {
    uint32_t vertexOffset = static_cast<uint32_t>(outMesh.Vertices.size());
    uint32_t indexOffset = static_cast<uint32_t>(outMesh.Indices.size());

    aiMatrix3x3 normalMatrix = aiMatrix3x3(transform);   // rotation/scale part, for transforming normals correctly
    normalMatrix.Inverse().Transpose();

    for (unsigned int i = 0; i < aiMeshData->mNumVertices; i++) {
        Vertex vertex{};

        aiVector3D pos = transform * aiMeshData->mVertices[i];   // apply accumulated node transform
        vertex.position = glm::vec3(pos.x, pos.y, pos.z);

        if (aiMeshData->HasNormals()) {
            aiVector3D norm = normalMatrix * aiMeshData->mNormals[i];
            norm.Normalize();
            vertex.normal = glm::vec3(norm.x, norm.y, norm.z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (aiMeshData->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(aiMeshData->mTextureCoords[0][i].x, aiMeshData->mTextureCoords[0][i].y);
        } else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        vertex.color = glm::vec3(1.0f);
        outMesh.Vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < aiMeshData->mNumFaces; i++) {
        aiFace face = aiMeshData->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            outMesh.Indices.push_back(face.mIndices[j] + vertexOffset);
        }
    }

    SubMesh sub;
    sub.indexOffset = indexOffset;
    sub.indexCount = static_cast<uint32_t>(outMesh.Indices.size()) - indexOffset;
    sub.materialIndex = static_cast<int>(aiMeshData->mMaterialIndex);
    outMesh.SubMeshes.push_back(sub);
}

Material ModelLoader::ProcessMaterial(const aiScene *scene, aiMaterial *aiMat, VulkanEngine *engine, MeshRenderer *meshRenderer) {
    Material material;

    aiString name;
    if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        material.name = name.C_Str();
    }

    aiColor4D baseColor;
    if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS) {
        material.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    } else if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS) {
        material.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    }

    // NEW — extract the embedded base-color texture, if one exists
    aiString texPath;
    bool hasTexture = (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS) ||
                      (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS);

if (hasTexture) {
        const aiTexture *embeddedTexture = scene->GetEmbeddedTexture(texPath.C_Str());
        if (embeddedTexture && embeddedTexture->mHeight == 0) {
            TextureData texData = TextureLoader::LoadFromMemory(
                    reinterpret_cast<const unsigned char *>(embeddedTexture->pcData),
                    embeddedTexture->mWidth);

            material.texture = std::make_shared<Texture>();
            material.texture->UploadToGPU(engine, texData);

            material.descriptorSet = meshRenderer->AllocateTextureDescriptorSet(
                    material.texture->imageView, material.texture->sampler);
        }
    }

    return material;
}
Skeleton ModelLoader::ExtractSkeleton(const aiScene *scene, aiMesh *mesh) {
    Skeleton skeleton;

    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone *aiboneData = mesh->mBones[i];

        Bone bone;
        bone.name = aiboneData->mName.C_Str();

        aiMatrix4x4 m = aiboneData->mOffsetMatrix;
        bone.inverseBindMatrix = glm::mat4(
                m.a1, m.b1, m.c1, m.d1,
                m.a2, m.b2, m.c2, m.d2,
                m.a3, m.b3, m.c3, m.d3,
                m.a4, m.b4, m.c4, m.d4);

        skeleton.boneNameToIndex[bone.name] = static_cast<int>(skeleton.bones.size());
        skeleton.bones.push_back(bone);
    }

    // Parent indices need the node hierarchy — find each bone's node, walk up to find its parent bone (if any)
    for (auto &bone : skeleton.bones) {
        aiNode *node = scene->mRootNode->FindNode(bone.name.c_str());
        if (node && node->mParent) {
            std::string parentName = node->mParent->mName.C_Str();
            auto it = skeleton.boneNameToIndex.find(parentName);
            if (it != skeleton.boneNameToIndex.end()) {
                bone.parentIndex = it->second;
            }
        }
    }

    std::cout << "Skeleton extracted: " << skeleton.bones.size() << " bones" << std::endl;
    for (auto &bone : skeleton.bones) {
        std::cout << "  Bone: " << bone.name << " (parent index: " << bone.parentIndex << ")" << std::endl;
    }

    return skeleton;
}

std::vector<AnimationClip> ModelLoader::ExtractAnimations(const aiScene *scene) {
    std::vector<AnimationClip> clips;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation *aianim = scene->mAnimations[i];

        AnimationClip clip;
        clip.name = aianim->mName.C_Str();
        clip.duration = static_cast<float>(aianim->mDuration);
        clip.ticksPerSecond = aianim->mTicksPerSecond != 0 ? static_cast<float>(aianim->mTicksPerSecond) : 25.0f;

        for (unsigned int j = 0; j < aianim->mNumChannels; j++) {
            aiNodeAnim *channel = aianim->mChannels[j];

            BoneAnimationTrack track;
            track.boneName = channel->mNodeName.C_Str();

            for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                auto &key = channel->mPositionKeys[k];
                track.positions.push_back({ static_cast<float>(key.mTime), glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z) });
            }
            for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                auto &key = channel->mRotationKeys[k];
                track.rotations.push_back({ static_cast<float>(key.mTime), glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z) });
            }
            for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                auto &key = channel->mScalingKeys[k];
                track.scales.push_back({ static_cast<float>(key.mTime), glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z) });
            }

            clip.tracks.push_back(track);
        }

        clips.push_back(clip);
    }

    std::cout << "Animations extracted: " << clips.size() << std::endl;
    for (auto &clip : clips) {
        std::cout << "  Clip: " << clip.name << " (" << clip.duration << " ticks @ " << clip.ticksPerSecond << " tps, "
                  << clip.tracks.size() << " bone tracks)" << std::endl;
    }

    return clips;
}

SkinnedMesh ModelLoader::LoadSkinnedModel(const std::string &filepath, VulkanEngine *engine, MeshRenderer *meshRenderer) {
    SkinnedMesh mesh;
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(filepath,
                                             aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
                                                     aiProcess_CalcTangentSpace);
    // NOTE: deliberately NOT using aiProcess_JoinIdenticalVertices here —
    // it can interfere with bone weight assignment in some Assimp versions
    // by merging vertices that have different bone influences. Revisit only
    // if you hit actual vertex-count bloat problems.

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::string error = "Failed to load skinned model: " + std::string(importer.GetErrorString());
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    std::cout << "Loaded skinned model: " << filepath << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Animations: " << scene->mNumAnimations << std::endl;

    // Materials — reuse the same ProcessMaterial you already have (assuming it doesn't need SkinnedMesh-specific changes)
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        mesh.Materials.push_back(ProcessMaterial(scene, scene->mMaterials[i], engine, meshRenderer));
    }

    // Geometry + bone weights — only process the first mesh for skeleton purposes,
    // since a single skeleton is typically shared across all sub-meshes of a rigged character
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        ProcessSkinnedMesh(scene, scene->mMeshes[i], aiMatrix4x4(), mesh);
    }

    // Skeleton — extracted from the first mesh that actually has bones
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        if (scene->mMeshes[i]->mNumBones > 0) {
            mesh.skeleton = ExtractSkeleton(scene, scene->mMeshes[i]);
            break;
        }
    }

    mesh.animations = ExtractAnimations(scene);

    std::cout << "  Vertices: " << mesh.Vertices.size() << std::endl;
    std::cout << "  Indices: " << mesh.Indices.size() << std::endl;
    std::cout << "  Bones: " << mesh.skeleton.bones.size() << std::endl;

    return mesh;
}

void ModelLoader::ProcessSkinnedMesh(const aiScene *scene, aiMesh *aiMeshData, const aiMatrix4x4 &transform, SkinnedMesh &outMesh) {
    uint32_t vertexOffset = static_cast<uint32_t>(outMesh.Vertices.size());
    uint32_t indexOffset = static_cast<uint32_t>(outMesh.Indices.size());

    // Build per-vertex bone influence lookup BEFORE the main vertex loop
    std::vector<std::vector<std::pair<int, float>>> vertexBoneData(aiMeshData->mNumVertices);
    for (unsigned int boneIdx = 0; boneIdx < aiMeshData->mNumBones; boneIdx++) {
        aiBone *bone = aiMeshData->mBones[boneIdx];
        for (unsigned int w = 0; w < bone->mNumWeights; w++) {
            unsigned int vertexId = bone->mWeights[w].mVertexId;
            float weight = bone->mWeights[w].mWeight;
            vertexBoneData[vertexId].push_back({ static_cast<int>(boneIdx), weight });
        }
    }

    aiMatrix3x3 normalMatrix = aiMatrix3x3(transform);
    normalMatrix.Inverse().Transpose();

    for (unsigned int i = 0; i < aiMeshData->mNumVertices; i++) {
        SkinnedVertex vertex{};

        aiVector3D pos = transform * aiMeshData->mVertices[i];
        vertex.position = glm::vec3(pos.x, pos.y, pos.z);

        if (aiMeshData->HasNormals()) {
            aiVector3D norm = normalMatrix * aiMeshData->mNormals[i];
            norm.Normalize();
            vertex.normal = glm::vec3(norm.x, norm.y, norm.z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (aiMeshData->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(aiMeshData->mTextureCoords[0][i].x, aiMeshData->mTextureCoords[0][i].y);
        } else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        vertex.color = glm::vec3(1.0f);

        auto &boneData = vertexBoneData[i];
        for (size_t b = 0; b < boneData.size() && b < 4; b++) {
            vertex.boneIndices[static_cast<int>(b)] = boneData[b].first;
            vertex.boneWeights[static_cast<int>(b)] = boneData[b].second;
            if (i == 0) {
                float sum = vertex.boneWeights.x + vertex.boneWeights.y + vertex.boneWeights.z + vertex.boneWeights.w;
                std::cout << "Vertex 0 weights: " << vertex.boneWeights.x << "," << vertex.boneWeights.y << ","
                    << vertex.boneWeights.z << "," << vertex.boneWeights.w << " sum=" << sum << std::endl;
            }
        }

        outMesh.Vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < aiMeshData->mNumFaces; i++) {
        aiFace face = aiMeshData->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            outMesh.Indices.push_back(face.mIndices[j] + vertexOffset);
        }
    }

    SubMesh sub;
    sub.indexOffset = indexOffset;
    sub.indexCount = static_cast<uint32_t>(outMesh.Indices.size()) - indexOffset;
    sub.materialIndex = static_cast<int>(aiMeshData->mMaterialIndex);
    outMesh.SubMeshes.push_back(sub);
}