#include <cassert>
#include <iostream>

#include "VulkanModel.hpp"
#include "VulkanObject.hpp"


namespace ve {

VulkanModel::VulkanModel(
	VulkanDevice& device,
	VertexVector const& vertices,
	IndexVector const& indices,
	uint32_t binding,
	VertexLayout layout
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
	VertexVector const& vertices,
	IndexVector const& instanceIndices,
	size_t nInstances,
	uint32_t binding,
	VertexLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexIndexBuffer(vertices, instanceIndices, nInstances);
}

VulkanModel::VulkanModel(
	VulkanDevice& device,
	std::vector<VertexVector*> const& vertices,
	IndexVector const& instanceIndices,
	size_t nInstances,
	uint32_t binding,
	VertexLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexIndexBuffer(vertices, instanceIndices, nInstances);
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

VkDeviceSize	VulkanModel::getModelSize(void) const noexcept
{
	if (this->isIndexed == true)
	{
		return this->vertexBuffer->getBufferSize() + this->indexBuffer->getBufferSize();
	}
	else
	{
		return this->vertexBuffer->getBufferSize();
	}
}

void	VulkanModel::createVertexBuffer(const VertexVector& vertices)
{
	this->vertexCount = static_cast<uint32_t>(vertices.size());
	assert(this->vertexCount >= 3UL && "Vertex count must be at least 3");

	size_t	vertexSize = 0UL;
	if (this->layout & VertexLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::TEXTURE) vertexSize += sizeof(vec2);
	assert(vertexSize > 0UL && "Empty layout for model");

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
		for (Vertex const& vertex : vertices)
		{
			if (this->layout & VertexLayout::VERTEX)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & VertexLayout::NORMAL)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & VertexLayout::TEXTURE)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offset);
				offset += sizeof(vec2);
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

void	VulkanModel::createIndexBuffer(const IndexVector& indices)
{
	this->indexCount = static_cast<uint32_t>(indices.size());
	assert(this->indexCount >= 3UL && "Index count must be at least 3");

	size_t		indexSize = sizeof(uint32_t);
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

	VkDeviceSize	bufferSize = indexSize * this->indexCount;
	this->vulkanDevice.copyBuffer(stagingBuffer.getBuffer(), this->indexBuffer->getBuffer(), bufferSize);
	this->isIndexed = true;
}

void	VulkanModel::createVertexIndexBuffer(VertexVector const& vertices, IndexVector const& instanceIndices, size_t nInstances)
{
	this->vertexCount += vertices.size();

	assert(this->vertexCount >= 3UL && "Vertex count must be at least 3");
	assert(this->vertexCount % nInstances == 0UL && "Mismatch between vertexes and number of instances");

	this->indexCount = instanceIndices.size() * nInstances;

	size_t vertexSize = 0UL;
	if (this->layout & VertexLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::TEXTURE) vertexSize += sizeof(vec2);
	assert(vertexSize > 0UL && "Empty layout for model");

	VulkanBuffer	stagingBufferVertex(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferVertex.map();

	size_t			indexSize = sizeof(uint32_t);
	VulkanBuffer	stagingBufferIndex(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferIndex.map();

	if (this->layout == DEFAULT_MODEL_LAYOUT)
	{
		uint32_t sizeData = this->vertexCount * vertexSize;
		stagingBufferVertex.writeToBuffer(static_cast<const void*>(vertices.data()), sizeData, 0);
	}
	else
	{
		uint32_t offsetVertex = 0U;
		for (Vertex const& vertex : vertices)
		{
			if (this->layout & VertexLayout::VERTEX)
			{
				stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offsetVertex);
				offsetVertex += sizeof(vec3);
			}
			if (this->layout & VertexLayout::NORMAL)
			{
				stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offsetVertex);
				offsetVertex += sizeof(vec3);
			}
			if (this->layout & VertexLayout::TEXTURE)
			{
				stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offsetVertex);
				offsetVertex += sizeof(vec2);
			}
		}
	}

	// index data doesn't 'exist' yet because the indexes depend
	// on the vertexes already inserted, each one is manually written inside the staging buffer
	uint32_t*	stagingIndexPtr = static_cast<uint32_t*>(stagingBufferIndex.getMappedMemory());
	uint32_t	nVertexForInstance = this->vertexCount / nInstances;
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

void	VulkanModel::createVertexIndexBuffer(std::vector<VertexVector*> const& vertices, IndexVector const& instanceIndices, size_t nInstances)
{
	for (VertexVector* worldVertexes : vertices)
	{
		this->vertexCount += worldVertexes->size();
	}
	assert(this->vertexCount >= 3UL && "Vertex count must be at least 3");
	assert(this->vertexCount % nInstances == 0UL && "Mismatch between vertexes and number of instances");

	this->indexCount = instanceIndices.size() * nInstances;

	size_t vertexSize = 0UL;
	if (this->layout & VertexLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & VertexLayout::TEXTURE) vertexSize += sizeof(vec2);
	assert(vertexSize > 0UL && "Empty layout for model");

	VulkanBuffer	stagingBufferVertex(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferVertex.map();

	size_t			indexSize = sizeof(uint32_t);
	VulkanBuffer	stagingBufferIndex(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		BUFFER_RAW
	);
	stagingBufferIndex.map();

	uint32_t offsetVertex = 0U;
	for (VertexVector* chunkVertexes : vertices) {
		// some chunks might be empty, skip them
		if (chunkVertexes->data() == nullptr) continue;

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
			for (Vertex const& vertex : *chunkVertexes)
			{
				if (this->layout & VertexLayout::VERTEX)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offsetVertex);
					offsetVertex += sizeof(vec3);
				}
				if (this->layout & VertexLayout::NORMAL)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offsetVertex);
					offsetVertex += sizeof(vec3);
				}
				if (this->layout & VertexLayout::TEXTURE)
				{
					stagingBufferVertex.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offsetVertex);
					offsetVertex += sizeof(vec2);
				}
			}
		}
	}

	// index data doesn't 'exist' yet because the indexes depend
	// on the vertexes already inserted, each one is manually written inside the staging buffer
	uint32_t*	stagingIndexPtr = static_cast<uint32_t*>(stagingBufferIndex.getMappedMemory());
	uint32_t	nVertexForInstance = this->vertexCount / nInstances;
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

MeshLayoutDescription	VulkanModel::getModelLayout(uint32_t binding, VertexLayout layout) noexcept
{
	MeshLayoutDescription data{};
	data.bindingConfig.resize(1);

	data.bindingConfig[0].binding = binding;
	data.bindingConfig[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	uint32_t locationIndex = 0U, offset = 0U;
	if (layout & VertexLayout::VERTEX)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & VertexLayout::NORMAL)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & VertexLayout::TEXTURE)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32_SFLOAT, offset}
		);
		offset += sizeof(vec2);
	}
	data.bindingConfig[0].stride = offset;
	return data;
}

}	// namespace ve
