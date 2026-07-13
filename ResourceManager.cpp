#include "ResourceManager.h"
#include "VulkanContext.h"
#include "Mesh.h"
#include "ModelLoader.h"
#include "Vertex.h"
#include <stdexcept>
#include <iostream>

ResourceManager::ResourceManager(VulkanContext* context)
	: m_Context(context)
{
}

ResourceManager::~ResourceManager() {
	Shutdown();
}

void ResourceManager::Init() {
	std::cout << "ResourceManager initialized" << std::endl;
}

void ResourceManager::Shutdown() {
	// Clear all meshes (unique_ptr handles cleanup)
	m_MeshCache.clear();
	std::cout << "ResourceManager shut down, " << m_MeshCache.size() << " meshes released" << std::endl;
}

Mesh* ResourceManager::LoadMesh(const std::string& filepath) {
	// Check if already loaded
	auto it = m_MeshCache.find(filepath);
	if (it != m_MeshCache.end()) {
		return it->second.get();
	}

	// Load new mesh using ModelLoader
	auto mesh = std::make_unique<Mesh>(ModelLoader::LoadModel(filepath));

	std::cout << "Loaded mesh: " << filepath << " (" 
			  << mesh->vertices.size() << " vertices, " 
			  << mesh->indices.size() << " indices)" << std::endl;

	return RegisterMesh(filepath, std::move(mesh));
}

Mesh* ResourceManager::LoadMeshAs(const std::string& filepath, const std::string& name) {
	// Check if name already exists
	if (HasMesh(name)) {
		std::cout << "Warning: Mesh '" << name << "' already exists, returning existing mesh" << std::endl;
		return GetMesh(name);
	}

	// Load the mesh
	auto mesh = std::make_unique<Mesh>(ModelLoader::LoadModel(filepath));

	std::cout << "Loaded mesh '" << name << "' from: " << filepath << std::endl;

	return RegisterMesh(name, std::move(mesh));
}

Mesh* ResourceManager::GetMesh(const std::string& name) {
	auto it = m_MeshCache.find(name);
	if (it != m_MeshCache.end()) {
		return it->second.get();
	}
	return nullptr;
}

bool ResourceManager::HasMesh(const std::string& name) const {
	return m_MeshCache.find(name) != m_MeshCache.end();
}

Mesh* ResourceManager::CreatePlane(const std::string& name, float width, float height) {
	if (HasMesh(name)) {
		return GetMesh(name);
	}

	auto mesh = std::make_unique<Mesh>();

	float hw = width * 0.5f;
	float hh = height * 0.5f;

	// Create a plane in the XZ plane (horizontal)
	mesh->vertices = {
		{{-hw, 0.0f, -hh}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{ hw, 0.0f, -hh}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ hw, 0.0f,  hh}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{-hw, 0.0f,  hh}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
	};

	mesh->indices = {
		0, 1, 2,
		2, 3, 0
	};

	std::cout << "Created plane mesh: " << name << std::endl;
	return RegisterMesh(name, std::move(mesh));
}

Mesh* ResourceManager::CreateCube(const std::string& name, float size) {
	if (HasMesh(name)) {
		return GetMesh(name);
	}

	auto mesh = std::make_unique<Mesh>();

	float hs = size * 0.5f; // half size

	// Cube vertices (24 vertices, 4 per face for proper normals and UVs)
	mesh->vertices = {
		// Front face (+Z)
		{{-hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{ hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{ hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

		// Back face (-Z)
		{{ hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
		{{-hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
		{{-hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
		{{ hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

		// Left face (-X)
		{{-hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{-hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{-hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{-hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

		// Right face (+X)
		{{ hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{ hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{ hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

		// Top face (+Y)
		{{-hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{ hs,  hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{-hs,  hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

		// Bottom face (-Y)
		{{-hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
		{{ hs, -hs, -hs}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
		{{-hs, -hs,  hs}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}
	};

	// Cube indices (6 faces * 2 triangles * 3 vertices)
	mesh->indices = {
		0, 1, 2, 2, 3, 0,       // Front
		4, 5, 6, 6, 7, 4,       // Back
		8, 9, 10, 10, 11, 8,    // Left
		12, 13, 14, 14, 15, 12, // Right
		16, 17, 18, 18, 19, 16, // Top
		20, 21, 22, 22, 23, 20  // Bottom
	};

	std::cout << "Created cube mesh: " << name << std::endl;
	return RegisterMesh(name, std::move(mesh));
}

Mesh* ResourceManager::CreateQuad(const std::string& name) {
	if (HasMesh(name)) {
		return GetMesh(name);
	}

	auto mesh = std::make_unique<Mesh>();

	// Create a quad for UI rendering (in screen space, -1 to 1)
	mesh->vertices = {
		{{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
	};

	mesh->indices = {
		0, 1, 2,
		2, 3, 0
	};

	std::cout << "Created quad mesh: " << name << std::endl;
	return RegisterMesh(name, std::move(mesh));
}

Mesh* ResourceManager::RegisterMesh(const std::string& name, std::unique_ptr<Mesh> mesh) {
	Mesh* meshPtr = mesh.get();
	m_MeshCache[name] = std::move(mesh);
	return meshPtr;
}
