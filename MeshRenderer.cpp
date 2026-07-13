#include "MeshRenderer.h"
#include "VulkanContext.h"
#include "Camera3D.h"
#include "Components.h"
#include "Mesh.h"
#include "Vertex.h"
#include <stdexcept>
#include <iostream>
#include <array>

MeshRenderer::MeshRenderer(VulkanContext* context)
	: m_Context(context)
{
}

MeshRenderer::~MeshRenderer() {
	Shutdown();
}

void MeshRenderer::Init() {
	CreateDescriptorSetLayout();
	CreateUniformBuffers();
	CreatePipeline();

	std::cout << "MeshRenderer initialized" << std::endl;
}

void MeshRenderer::Shutdown() {
	VkDevice device = m_Context->GetDevice();

	// Cleanup uniform buffers
	for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
		if (m_UniformBuffersMapped[i]) {
			vkUnmapMemory(device, m_UniformBuffersMemory[i]);
		}
		vkDestroyBuffer(device, m_UniformBuffers[i], nullptr);
		vkFreeMemory(device, m_UniformBuffersMemory[i], nullptr);
	}

	// Cleanup pipeline
	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, m_Pipeline, nullptr);
	}
	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}

	// Cleanup descriptors
	if (m_DescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
	}
	if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
	}

	std::cout << "MeshRenderer shut down" << std::endl;
}

void MeshRenderer::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_Context->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void MeshRenderer::CreatePipeline() {
	// Push constant range for model matrix and color
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstants);

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Load shaders
	auto vertCode = m_Context->ReadFile("Shaders/mesh_3d_vert.spv");
	auto fragCode = m_Context->ReadFile("Shaders/mesh_3d_frag.spv");

	VkShaderModule vertShaderModule = m_Context->CreateShaderModule(vertCode);
	VkShaderModule fragShaderModule = m_Context->CreateShaderModule(fragCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// Vertex input
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkExtent2D extent = m_Context->GetSwapChainExtent();
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = m_Context->GetRenderPass();
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(m_Context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(m_Context->GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(m_Context->GetDevice(), vertShaderModule, nullptr);

	std::cout << "MeshRenderer pipeline created successfully" << std::endl;
}

void MeshRenderer::CreateUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(CameraUBO);

	// Create one uniform buffer per frame in flight (typically 2)
	size_t frameCount = 2; // MAX_FRAMES_IN_FLIGHT
	m_UniformBuffers.resize(frameCount);
	m_UniformBuffersMemory.resize(frameCount);
	m_UniformBuffersMapped.resize(frameCount);

	for (size_t i = 0; i < frameCount; i++) {
		m_Context->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_UniformBuffers[i],
			m_UniformBuffersMemory[i]
		);

		// Map the buffer memory so we can write to it
		vkMapMemory(m_Context->GetDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
	}

	// Create descriptor pool
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(frameCount);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(frameCount);

	if (vkCreateDescriptorPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	// Create descriptor sets
	std::vector<VkDescriptorSetLayout> layouts(frameCount, m_DescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(frameCount);
	allocInfo.pSetLayouts = layouts.data();

	m_DescriptorSets.resize(frameCount);
	if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// Update descriptor sets
	for (size_t i = 0; i < frameCount; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraUBO);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);
	}
}

void MeshRenderer::UpdateUniformBuffer(uint32_t frameIndex, const glm::mat4& view, const glm::mat4& projection) {
	CameraUBO ubo{};
	ubo.view = view;
	ubo.projection = projection;

	memcpy(m_UniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}

void MeshRenderer::UploadMesh(Mesh& mesh) {
	if (mesh.vertices.empty()) {
		std::cout << "Warning: Attempting to upload empty mesh" << std::endl;
		return;
	}

	VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
	VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

	// Create staging buffer for vertices
	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingBufferMemory;
	m_Context->CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexStagingBuffer,
		vertexStagingBufferMemory
	);

	void* data;
	vkMapMemory(m_Context->GetDevice(), vertexStagingBufferMemory, 0, vertexBufferSize, 0, &data);
	memcpy(data, mesh.vertices.data(), vertexBufferSize);
	vkUnmapMemory(m_Context->GetDevice(), vertexStagingBufferMemory);

	// Create vertex buffer on GPU
	m_Context->CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh.vertexBuffer,
		mesh.vertexBufferMemory
	);

	m_Context->CopyBuffer(vertexStagingBuffer, mesh.vertexBuffer, vertexBufferSize);

	// Cleanup staging buffer
	vkDestroyBuffer(m_Context->GetDevice(), vertexStagingBuffer, nullptr);
	vkFreeMemory(m_Context->GetDevice(), vertexStagingBufferMemory, nullptr);

	// Create staging buffer for indices
	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingBufferMemory;
	m_Context->CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexStagingBuffer,
		indexStagingBufferMemory
	);

	vkMapMemory(m_Context->GetDevice(), indexStagingBufferMemory, 0, indexBufferSize, 0, &data);
	memcpy(data, mesh.indices.data(), indexBufferSize);
	vkUnmapMemory(m_Context->GetDevice(), indexStagingBufferMemory);

	// Create index buffer on GPU
	m_Context->CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh.indexBuffer,
		mesh.indexBufferMemory
	);

	m_Context->CopyBuffer(indexStagingBuffer, mesh.indexBuffer, indexBufferSize);

	// Cleanup staging buffer
	vkDestroyBuffer(m_Context->GetDevice(), indexStagingBuffer, nullptr);
	vkFreeMemory(m_Context->GetDevice(), indexStagingBufferMemory, nullptr);

	std::cout << "Uploaded mesh to GPU (" << mesh.vertices.size() << " vertices, " 
			  << mesh.indices.size() << " indices)" << std::endl;
}

void MeshRenderer::Render(entt::registry& registry, const Camera3D& camera) {
	// Update camera uniform buffer
	uint32_t frameIndex = m_Context->GetCurrentFrameIndex();
	UpdateUniformBuffer(frameIndex, camera.GetViewMatrix(), camera.GetProjectionMatrix());

	VkCommandBuffer commandBuffer = m_Context->GetCurrentCommandBuffer();

	// Bind pipeline
	if (m_Pipeline != VK_NULL_HANDLE) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	// Bind descriptor set (camera UBO)
	if (m_PipelineLayout != VK_NULL_HANDLE && !m_DescriptorSets.empty()) {
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_PipelineLayout,
			0, // first set
			1, // descriptor set count
			&m_DescriptorSets[frameIndex],
			0, // dynamic offset count
			nullptr // dynamic offsets
		);
	}

	// Render all entities with Transform and MeshComponent
	auto view = registry.view<Transform, MeshComponent>();
	for (auto entity : view) {
		auto& transform = view.get<Transform>(entity);
		auto& meshComp = view.get<MeshComponent>(entity);

		if (!meshComp.mesh || meshComp.mesh->vertices.empty()) {
			continue;
		}

		Mesh* mesh = meshComp.mesh;

		// Ensure mesh is uploaded to GPU
		if (mesh->vertexBuffer == VK_NULL_HANDLE) {
			UploadMesh(*mesh);
		}

		// Push constants for model matrix
		PushConstants pushConstants{};
		pushConstants.model = transform.GetModelMatrix();
		pushConstants.color = glm::vec4(1.0f); // Default white

		// Check if entity has MaterialComponent for color
		if (registry.all_of<MaterialComponent>(entity)) {
			auto& material = registry.get<MaterialComponent>(entity);
			pushConstants.color = material.color;
		}

		if (m_PipelineLayout != VK_NULL_HANDLE) {
			vkCmdPushConstants(
				commandBuffer,
				m_PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstants),
				&pushConstants
			);
		}

		// Bind vertex and index buffers
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Draw
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
	}
}
