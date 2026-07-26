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

    // Load all materials up front
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        mesh.Materials.push_back(ProcessMaterial(scene->mMaterials[i]));
    }

    // Load each mesh as its own sub-mesh, appending into the shared vertex/index buffers
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        ProcessMesh(scene->mMeshes[i], mesh);
    }

    std::cout << "  Vertices: " << mesh.Vertices.size() << std::endl;
    std::cout << "  Indices: " << mesh.Indices.size() << std::endl;
    std::cout << "  SubMeshes: " << mesh.SubMeshes.size() << std::endl;

    return mesh;
}

Material ModelLoader::ProcessMaterial(aiMaterial* aiMat) {
    Material material;

    aiString name;
    if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        material.name = name.C_Str();
    }

    aiColor4D baseColor;
    if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS) {
        material.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    }
    else if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS) {
        // Fallback for older-style materials (OBJ, FBX, etc.)
        material.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    }
    std::cout << "Material '" << material.name << "' baseColor: "
        << material.baseColor.r << ", " << material.baseColor.g << ", "
        << material.baseColor.b << ", " << material.baseColor.a << std::endl;


    return material;
}

void ModelLoader::ProcessMesh(aiMesh* aiMeshData, Mesh& outMesh) {
    uint32_t vertexOffset = static_cast<uint32_t>(outMesh.Vertices.size());
    uint32_t indexOffset = static_cast<uint32_t>(outMesh.Indices.size());

    for (unsigned int i = 0; i < aiMeshData->mNumVertices; i++) {
        Vertex vertex{};
        vertex.position = glm::vec3(aiMeshData->mVertices[i].x, aiMeshData->mVertices[i].y, aiMeshData->mVertices[i].z);

        if (aiMeshData->HasNormals()) {
            vertex.normal = glm::vec3(aiMeshData->mNormals[i].x, aiMeshData->mNormals[i].y, aiMeshData->mNormals[i].z);
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (aiMeshData->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(aiMeshData->mTextureCoords[0][i].x, aiMeshData->mTextureCoords[0][i].y);
        }
        else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        vertex.color = glm::vec3(1.0f);   // material color now drives appearance via push constant, not vertex color
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
