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


// ModelLoader.cpp
Mesh ModelLoader::LoadModel(const std::string& filepath) {
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
        mesh.Materials.push_back(ProcessMaterial(scene->mMaterials[i]));
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

Material ModelLoader::ProcessMaterial(aiMaterial *aiMat) {
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

    return material;
}