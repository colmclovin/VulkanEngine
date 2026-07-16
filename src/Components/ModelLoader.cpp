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

Mesh ModelLoader::LoadModel(const std::string &filepath) {
    Mesh mesh;

    // Assimp::Importer handles loading different file formats
    Assimp::Importer importer;

    // ReadFile loads the model and applies post-processing
    // Works with .obj, .fbx, .glb, .gltf, .dae, etc.
    const aiScene *scene = importer.ReadFile(filepath,
                                             aiProcess_Triangulate | // Convert all polygons to triangles
                                                     aiProcess_FlipUVs | // Flip texture coordinates for Vulkan
                                                     aiProcess_GenNormals | // Generate normals if missing
                                                     aiProcess_JoinIdenticalVertices | // Optimize by merging duplicate vertices
                                                     aiProcess_CalcTangentSpace); // Calculate tangents for normal mapping

    // Error checking
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::string error = "Failed to load model: " + std::string(importer.GetErrorString());
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    std::cout << "Loaded model: " << filepath << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Materials: " << scene->mNumMaterials << std::endl;

    // For now, just load the first mesh
    // In a real application, you'd loop through all meshes
    if (scene->mNumMeshes > 0) {
        ProcessMesh(scene->mMeshes[0], mesh);
        std::cout << "  Vertices: " << mesh.Vertices.size() << std::endl;
        std::cout << "  Indices: " << mesh.Indices.size() << std::endl;
    }

    return mesh;
};

void ModelLoader::ProcessMesh(aiMesh *aiMeshData, Mesh &outMesh) {
    // aiMesh is Assimp's mesh structure containing:
    // - mVertices: array of vertex positions
    // - mNormals: array of normal vectors
    // - mTextureCoords: array of UV coordinates
    // - mFaces: array of faces (triangles if we used aiProcess_Triangulate)

    // Load vertices
    for (unsigned int i = 0; i < aiMeshData->mNumVertices; i++) {
        Vertex vertex{};

        // Position - aiMeshData->mVertices is an array of aiVector3D
        vertex.position = glm::vec3(
                aiMeshData->mVertices[i].x,
                aiMeshData->mVertices[i].y,
                aiMeshData->mVertices[i].z);

        // Normal
        if (aiMeshData->HasNormals()) {
            vertex.normal = glm::vec3(
                    aiMeshData->mNormals[i].x,
                    aiMeshData->mNormals[i].y,
                    aiMeshData->mNormals[i].z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up
        }

        // Texture coordinates (UV)
        // Assimp supports up to 8 UV channels, we use the first one
        if (aiMeshData->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(
                    aiMeshData->mTextureCoords[0][i].x,
                    aiMeshData->mTextureCoords[0][i].y);
        } else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        // Default white color (you can later get this from materials)
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

        outMesh.Vertices.push_back(vertex);
    }

    // Load indices
    // aiFace represents a polygon face (triangle after aiProcess_Triangulate)
    for (unsigned int i = 0; i < aiMeshData->mNumFaces; i++) {
        aiFace face = aiMeshData->mFaces[i];
        // Each face should have 3 indices (since we triangulated)
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            outMesh.Indices.push_back(face.mIndices[j]);
        }
    }
};