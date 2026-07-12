#include "Renderer2D.h"
#include "Camera2D.h"
#include "VulkanEngine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// Vertex structure for batched rendering
struct QuadVertex {
	glm::vec3 position;
	glm::vec4 color;
	glm::vec2 texCoord;
	float texIndex; // texture slot index
};

// Renderer data (internal state)
struct RendererData {
	static const uint32_t MaxQuads = 10000;
	static const uint32_t MaxVertices = MaxQuads * 4;
	static const uint32_t MaxIndices = MaxQuads * 6;

	VulkanEngine* vulkanEngine = nullptr;

	// Batch data
	std::vector<QuadVertex> quadVertexBuffer;
	uint32_t quadIndexCount = 0;

	// Current scene
	glm::mat4 viewProjectionMatrix;

	// Statistics
	RendererStats stats;
};

static RendererData s_Data;

void Renderer2D::Init(VulkanEngine* engine) {
	s_Data.vulkanEngine = engine;
	s_Data.quadVertexBuffer.reserve(RendererData::MaxVertices);

	// Initialize stats
	s_Data.stats.Reset();
}

void Renderer2D::Shutdown() {
	s_Data.quadVertexBuffer.clear();
}

void Renderer2D::BeginScene(const Camera2D& camera) {
	s_Data.viewProjectionMatrix = camera.GetViewProjectionMatrix();

	// Reset batch
	s_Data.quadVertexBuffer.clear();
	s_Data.quadIndexCount = 0;
}

void Renderer2D::EndScene() {
	Flush();
}

void Renderer2D::Flush() {
	if (s_Data.quadIndexCount == 0)
		return; // Nothing to draw

	// Generate indices for quads (2 triangles per quad)
	std::vector<uint32_t> indices;
	indices.reserve(s_Data.quadIndexCount);

	uint32_t quadCount = s_Data.quadIndexCount / 6;
	for (uint32_t i = 0; i < quadCount; i++) {
		uint32_t baseVertex = i * 4;

		// First triangle (0, 1, 2)
		indices.push_back(baseVertex + 0);
		indices.push_back(baseVertex + 1);
		indices.push_back(baseVertex + 2);

		// Second triangle (2, 3, 0)
		indices.push_back(baseVertex + 2);
		indices.push_back(baseVertex + 3);
		indices.push_back(baseVertex + 0);
	}

	// Submit batch to VulkanEngine
	VulkanEngine::RenderBatch batch;
	batch.vertices = s_Data.quadVertexBuffer.data();
	batch.vertexCount = static_cast<uint32_t>(s_Data.quadVertexBuffer.size());
	batch.indices = indices.data();
	batch.indexCount = static_cast<uint32_t>(indices.size());
	batch.viewProjectionMatrix = &s_Data.viewProjectionMatrix[0][0]; // Get pointer to matrix data

	s_Data.vulkanEngine->SubmitRenderBatch(batch);

	// Update stats
	s_Data.stats.drawCalls++;
	s_Data.stats.quadCount = s_Data.quadIndexCount / 6;
	s_Data.stats.vertexCount = s_Data.quadVertexBuffer.size();
	s_Data.stats.indexCount = s_Data.quadIndexCount;

	// Reset batch
	s_Data.quadVertexBuffer.clear();
	s_Data.quadIndexCount = 0;
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
	DrawQuad(glm::vec3(position, 0.0f), size, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) {
	// Check if we need to flush
	if (s_Data.quadIndexCount >= RendererData::MaxIndices) {
		Flush();
		BeginScene(*reinterpret_cast<Camera2D*>(nullptr)); // HACK: Need to store camera
	}

	// Create transform matrix
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
						  glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

	// Quad vertices (local space)
	std::array<glm::vec4, 4> quadVertices = {
		glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
		glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f),
		glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
		glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f)
	};

	// Texture coordinates
	std::array<glm::vec2, 4> texCoords = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(0.0f, 1.0f)
	};

	// Add vertices to batch
	for (size_t i = 0; i < 4; i++) {
		QuadVertex vertex;
		vertex.position = transform * quadVertices[i];
		vertex.color = color;
		vertex.texCoord = texCoords[i];
		vertex.texIndex = 0.0f; // White texture
		s_Data.quadVertexBuffer.push_back(vertex);
	}

	s_Data.quadIndexCount += 6;
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color) {
	DrawRotatedQuad(position, size, rotation, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color) {
	// Check if we need to flush
	if (s_Data.quadIndexCount >= RendererData::MaxIndices) {
		Flush();
	}

	// Create transform matrix with rotation
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) *
						  glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
						  glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

	// Quad vertices
	std::array<glm::vec4, 4> quadVertices = {
		glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
		glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f),
		glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
		glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f)
	};

	std::array<glm::vec2, 4> texCoords = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(0.0f, 1.0f)
	};

	for (size_t i = 0; i < 4; i++) {
		QuadVertex vertex;
		vertex.position = transform * quadVertices[i];
		vertex.color = color;
		vertex.texCoord = texCoords[i];
		vertex.texIndex = 0.0f;
		s_Data.quadVertexBuffer.push_back(vertex);
	}

	s_Data.quadIndexCount += 6;
}

void Renderer2D::DrawSprite(const Sprite& sprite) {
	DrawRotatedQuad(sprite.position, sprite.size, sprite.rotation, sprite.color);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, uint32_t textureID) {
	// TODO: Implement texture support
	DrawQuad(position, size, glm::vec4(1.0f));
}

uint32_t Renderer2D::LoadTexture(const char *path) {
	// TODO: Implement texture loading with stb_image
	    int x,y,n;
    unsigned char *data = stbi_load(path, &x, &y, &n, 0);
    // ... process data if not NULL ...
    // ... x = width, y = height, n = # 8-bit components per pixel ...
    // ... replace '0' with '1'..'4' to force that many components per pixel
    // ... but 'n' will always be the number that it would have been if you said 0
    stbi_image_free(data);
	return 0;
}

RendererStats Renderer2D::GetStats() {
	return s_Data.stats;
}

void Renderer2D::ResetStats() {
	s_Data.stats.Reset();
}
