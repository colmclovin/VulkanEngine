#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>

// Forward declarations
class VulkanEngine;
class Camera2D;

// Simple sprite structure
struct Sprite {
	glm::vec2 position;
	glm::vec2 size;
	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float rotation = 0.0f; // in radians
	uint32_t textureID = 0; // 0 = white square
};

// Renderer statistics
struct RendererStats {
	uint32_t drawCalls = 0;
	uint32_t quadCount = 0;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	void Reset() {
		drawCalls = 0;
		quadCount = 0;
		vertexCount = 0;
		indexCount = 0;
	}
};

// Main 2D renderer class
class Renderer2D {
public:
	// Initialization
	static void Init(VulkanEngine* engine);
	static void Shutdown();

	// Frame management
	static void BeginScene(const Camera2D& camera);
	static void EndScene();
	static void Flush(); // Force draw current batch

	// Drawing API
	static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
	static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
	static void DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);

	static void DrawSprite(const Sprite& sprite);
	static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);

	// Texture management
	static uint32_t LoadTexture(const char *path);
	static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, uint32_t textureID);

	// Statistics
	static RendererStats GetStats();
	static void ResetStats();

private:
	Renderer2D() = delete; // Static class only
};
