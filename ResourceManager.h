#pragma once
#include <string>
#include <unordered_map>
#include <memory>

// Forward declarations
class VulkanContext;
class Mesh;

// ============================================================================
// ResourceManager - Centralized asset loading and caching
// ============================================================================
class ResourceManager {
public:
	ResourceManager(VulkanContext* context);
	~ResourceManager();

	void Init();
	void Shutdown();

	// Mesh loading and retrieval
	Mesh* LoadMesh(const std::string& filepath);
	Mesh* GetMesh(const std::string& name);
	bool HasMesh(const std::string& name) const;

	// Primitive mesh creation (procedural geometry)
	Mesh* CreatePlane(const std::string& name, float width = 1.0f, float height = 1.0f);
	Mesh* CreateCube(const std::string& name, float size = 1.0f);
	Mesh* CreateQuad(const std::string& name); // For UI rendering

	// Utility to create a mesh from an existing file but give it a specific name
	Mesh* LoadMeshAs(const std::string& filepath, const std::string& name);

private:
	VulkanContext* m_Context = nullptr;
	std::unordered_map<std::string, std::unique_ptr<Mesh>> m_MeshCache;

	// Helper to register a mesh in the cache
	Mesh* RegisterMesh(const std::string& name, std::unique_ptr<Mesh> mesh);
};
