#include <cassert>
#include <iostream>

#include "VulkanModel.hpp"
#include "VulkanObject.hpp"


namespace ve {

VulkanModel::VulkanModel(
	VulkanDevice& device,
	std::vector<Vertex> const& vertices,
	std::vector<uint32_t> const& indices,
	uint32_t binding,
	MeshLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexBuffer(vertices);
	if (indices.size() > 2U)
	{
		this->createIndexBuffer(indices);
	}
}

VulkanModel::VulkanModel(
	VulkanDevice& device,
	std::vector<std::vector<Vertex>*> const& vertices,
	MeshType type,
	uint32_t binding,
	MeshLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexIndexBuffer(vertices, type);
}

void	VulkanModel::bindBuffer(VkCommandBuffer commandBuffer) const noexcept
{
	VkBuffer		buffer = this->vertexBuffer->getBuffer();
	VkDeviceSize	offset = 0UL;

	vkCmdBindVertexBuffers(commandBuffer, this->binding, 1, &buffer, &offset);
	if (this->isIndexed == true)
	{
		buffer = this->indexBuffer->getBuffer();
		vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
	}
}

void	VulkanModel::draw(VkCommandBuffer commandBuffer) const noexcept
{
	if (this->isIndexed == true)
	{
		vkCmdDrawIndexed(commandBuffer, this->indexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, this->vertexCount, 1, 0, 0);
	}
}

void	VulkanModel::createVertexBuffer(const std::vector<Vertex>& vertices)
{
	this->vertexCount = static_cast<uint32_t>(vertices.size());
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");

	uint32_t	vertexSize = 0U;
	if (this->layout & MeshLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::TEXTURE) vertexSize += sizeof(vec2);
	if (this->layout & MeshLayout::RANDOM_INDEX_TEXT) vertexSize += sizeof(uint32_t);
	assert(vertexSize > 0U && "Empty layout for model");

	VulkanBuffer	stagingBuffer(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);
	stagingBuffer.map();

	if (this->layout == DEFAULT_MODEL_LAYOUT)
	{
		stagingBuffer.writeToBuffer(static_cast<const void*>(vertices.data()));
	}
	else
	{
		uint32_t offset = 0U;
		for (VulkanModel::Vertex const& vertex : vertices)
		{
			if (this->layout & MeshLayout::VERTEX)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & MeshLayout::NORMAL)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & MeshLayout::TEXTURE)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offset);
				offset += sizeof(vec2);
			}
			if (this->layout & MeshLayout::RANDOM_INDEX_TEXT)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureIndex), sizeof(uint32_t), offset);
				offset += sizeof(uint32_t);
			}
		}
	}
	stagingBuffer.flush();

	this->vertexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_VERTEX
	);

	VkDeviceSize	bufferSize = vertexSize * this->vertexCount;
	this->vulkanDevice.copyBuffer(stagingBuffer.getBuffer(), this->vertexBuffer->getBuffer(), bufferSize);
}

void	VulkanModel::createIndexBuffer(const std::vector<uint32_t>& indices)
{
	this->indexCount = static_cast<uint32_t>(indices.size());
	assert(this->indexCount >= 3 && "Index count must be at least 3");

	uint32_t		indexSize = sizeof(uint32_t);
	VkDeviceSize	bufferSize = indexSize * this->indexCount;

	VulkanBuffer	stagingBuffer(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);

	stagingBuffer.map();
	stagingBuffer.writeToBuffer(static_cast<const void*>(indices.data()));
	stagingBuffer.flush();

	this->indexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_INDEX
	);
	this->vulkanDevice.copyBuffer(stagingBuffer.getBuffer(), this->indexBuffer->getBuffer(), bufferSize);
	this->isIndexed = true;
}

void	VulkanModel::createVertexIndexBuffer(std::vector<std::vector<Vertex>*> const& vertices, MeshType type)
{
	for (std::vector<Vertex>* worldVertexes : vertices)
	{
		this->vertexCount += worldVertexes->size();
	}
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");

	// depending on the type of the instances, use a 'template' set of indexes: face-> 6 indexes, cube-> 36 indexes 
	std::vector<uint32_t>	instanceIndices;
	uint32_t				nVertexForInstance = 0U, nInstances = 0U;
	if (type == MeshType::VOXEL)
	{
		assert(this->vertexCount % VERTEX_PER_VOXEL == 0U && "Vertexes represent voxels but the number is not multiple of 8");
		// a voxel has always 24 vertexes and 36 indexes, with this proportion, given
		// an amount of voxels, the total number of indexes is: nVoxels * nIndexPerVoxel / nVertexPerVoxel
		nVertexForInstance = VERTEX_PER_VOXEL;
		nInstances = this->vertexCount / nVertexForInstance;
		this->indexCount = nInstances * INDEX_PER_VOXEL;
		instanceIndices.insert(instanceIndices.begin(), VOXEL_INDEXES.begin(), VOXEL_INDEXES.end());
	}
	else if (type == MeshType::FACE)
	{
		assert(this->vertexCount % VERTEX_PER_FACE == 0U && "Vertexes represent faces but the number is not multiple of 6");
		// a face has always 4 vertexes and 6 indexes, with this proportion, given
		// an amount of faces, the total number of indexes is: nFaces * nIndexPerFace / nVertexPerFace
		nVertexForInstance = VERTEX_PER_FACE;
		nInstances = this->vertexCount / nVertexForInstance;
		this->indexCount = nInstances * INDEX_PER_FACE;
		instanceIndices.insert(instanceIndices.begin(), FACE_INDEXES.begin(), FACE_INDEXES.end());
	}

	uint32_t vertexSize = 0U;
	if (this->layout & MeshLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::TEXTURE) vertexSize += sizeof(vec2);
	if (this->layout & MeshLayout::RANDOM_INDEX_TEXT) vertexSize += sizeof(uint32_t);
	assert(vertexSize > 0U && "Empty layout for model");

	VulkanBuffer	stagingBufferVertex(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferVertex.map();

	uint32_t		indexSize = sizeof(uint32_t);
	VulkanBuffer	stagingBufferIndex(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferIndex.map();

	uint32_t offsetVertex = 0U;		// this is a byte offset
	for (std::vector<Vertex>* chunkVertexes : vertices) {
		// some chunks might be empty, skip them
		if (chunkVertexes->data() == nullptr)
		{
			continue;
		}
		// insert vertexes of this chunk in the staging buffer
		uint32_t nVertexes = chunkVertexes->size();
		if (this->layout == DEFAULT_MODEL_LAYOUT)
		{
			uint32_t sizeData = nVertexes * vertexSize;
			stagingBufferVertex.writeToBuffer(static_cast<const void*>(chunkVertexes->data()), sizeData, offsetVertex);
			offsetVertex += sizeData;
		}
		else
		{
			// uint32_t offset = 0U;
			for (VulkanModel::Vertex const& vertex : *chunkVertexes)
			{
				if (this->layout & MeshLayout::VERTEX)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offsetVertex);
					offsetVertex += sizeof(vec3);
				}
				if (this->layout & MeshLayout::NORMAL)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offsetVertex);
					offsetVertex += sizeof(vec3);
				}
				if (this->layout & MeshLayout::TEXTURE)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offsetVertex);
					offsetVertex += sizeof(vec2);
				}
				if (this->layout & MeshLayout::RANDOM_INDEX_TEXT)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.textureIndex), sizeof(uint32_t), offsetVertex);
					offsetVertex += sizeof(uint32_t);
				}
			}
		}
	}

	// index data doesn't 'exist' yet because the indexes depend
	// on the vertexes already inserted, each one is manually written inside the staging buffer
	uint32_t*	stagingIndexPtr = static_cast<uint32_t*>(stagingBufferIndex.getMappedMemory());
	for (uint32_t i = 0; i < nInstances; i++)
	{
		for (uint32_t index : instanceIndices)
		{
			*stagingIndexPtr = index + i * nVertexForInstance;
			stagingIndexPtr++;
		}
	}

	this->vertexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_VERTEX
	);
	this->vulkanDevice.copyBuffer(stagingBufferVertex.getBuffer(), this->vertexBuffer->getBuffer(), this->vertexCount * vertexSize);

	this->indexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_INDEX
	);
	this->vulkanDevice.copyBuffer(stagingBufferIndex.getBuffer(), this->indexBuffer->getBuffer(), this->indexCount * indexSize);
	this->isIndexed = true;
}

MeshLayoutDescription	VulkanModel::getModelLayout(uint32_t binding, MeshLayout layout) noexcept
{
	MeshLayoutDescription data{};
	data.bindingConfig.resize(1);

	data.bindingConfig[0].binding = binding;
	data.bindingConfig[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	uint32_t locationIndex = 0U, offset = 0U;
	if (layout & MeshLayout::VERTEX)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & MeshLayout::NORMAL)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & MeshLayout::TEXTURE)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32_SFLOAT, offset}
		);
		offset += sizeof(vec2);
	}
	if (layout & MeshLayout::RANDOM_INDEX_TEXT)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32_UINT, offset}
		);
		offset += sizeof(uint32_t);
	}
	data.bindingConfig[0].stride = offset;
	return data;
}

}	// namespace ve
