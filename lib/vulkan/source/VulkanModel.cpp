#include "VulkanModel.hpp"
#include "VulkanObject.hpp"

#include <cassert>


namespace ve {

VulkanModel::VulkanModel(
	VulkanDevice& device,
	const Builder& builder,
	uint32_t binding,
	MeshLayout type
) :
	vulkanDevice{device}, binding{binding}, type{type}
{
	this->createVertexBuffers(builder.vertices);
	if (builder.indices.size() > 2U)
	{
		this->createIndexBuffers(builder.indices);
	}
}

VulkanModel::VulkanModel(
	VulkanDevice& device,
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices,
	uint32_t binding,
	MeshLayout type
) :
	vulkanDevice{device}, binding{binding}, type{type}
{
	this->createVertexBuffers(vertices);
	if (indices.size() > 2U)
	{
		this->createIndexBuffers(indices);
	}
}

VulkanModel::VulkanModel(
	VulkanDevice& device,
	const std::vector<std::vector<Vertex>>& vertices,
	const std::array<uint32_t,INDEX_PER_VOXEL>& indexesVoxel,
	uint32_t binding,
	MeshLayout type
) :
	vulkanDevice{device}, binding{binding}, type{type}
{
	for (std::vector<Vertex> const& worldVertexes : vertices)
	{
		this->vertexCount += worldVertexes.size();
		// a voxel has always 24 vertexes and 36 indexes, with this proportion, given
		// an amount of voxels, the total number of indexes is: nVoxels * nIndexPerVoxel / nVertexPerVoxel
		this->indexCount += (worldVertexes.size() * INDEX_PER_VOXEL) / VERTEX_PER_VOXEL;
	}
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");
	this->createVertexIndexBuffers(vertices, indexesVoxel);
}

void	VulkanModel::bindBuffer(VkCommandBuffer commandBuffer) const noexcept
{
	VkBuffer		buffers[] = {this->vertexBuffer->getBuffer()};
	VkDeviceSize	offsets[] = {0};

	vkCmdBindVertexBuffers(commandBuffer, this->binding, 1, buffers, offsets);
	if (this->isIndexed == true)
	{
		vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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


void	VulkanModel::setBoundingBox(const std::vector<Vertex>& vertices) noexcept
{
	this->boundingBox.min.x = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.x < b.pos.x; })->pos.x;
	this->boundingBox.max.x = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.x < b.pos.x; })->pos.x;
	this->boundingBox.min.y = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.y < b.pos.y; })->pos.y;
	this->boundingBox.max.y = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.y < b.pos.y; })->pos.y;
	this->boundingBox.min.z = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.z < b.pos.z; })->pos.z;
	this->boundingBox.max.z = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.z < b.pos.z; })->pos.z;
}

void	VulkanModel::createVertexBuffers(const std::vector<Vertex>& vertices)
{
	this->vertexCount = static_cast<uint32_t>(vertices.size());
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");

	uint32_t	vertexSize = 0U;
	if (this->type & MeshLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->type & MeshLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->type & MeshLayout::TEXTURE) vertexSize += sizeof(vec2);
	if (this->type & MeshLayout::RANDOM_INDEX_TEXT) vertexSize += sizeof(uint32_t);
	assert(vertexSize > 0U && "Empty layout for model");

	VulkanBuffer	stagingBuffer(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);
	stagingBuffer.map();

	if (this->type == DEFAULT_MODEL_LAYOUT)
	{
		stagingBuffer.writeToBuffer(static_cast<const void*>(vertices.data()));
	}
	else
	{
		uint32_t offset = 0U;
		for (VulkanModel::Vertex const& vertex : vertices)
		{
			if (this->type & MeshLayout::VERTEX)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->type & MeshLayout::NORMAL)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->type & MeshLayout::TEXTURE)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offset);
				offset += sizeof(vec2);
			}
			if (this->type & MeshLayout::RANDOM_INDEX_TEXT)
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

void	VulkanModel::createIndexBuffers(const std::vector<uint32_t>& indices)
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

void	VulkanModel::createVertexIndexBuffers(const std::vector<std::vector<Vertex>>& vertices, const std::array<uint32_t, INDEX_PER_VOXEL>& indexesVoxel)
{
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");
	assert(this->indexCount >= 3 && "Index count must be at least 3");

	uint32_t		vertexSize = sizeof(Vertex);
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
	// the face index data doesn't 'exist' yet because the indexes depend
	// on the vertexes already inserted, each one is manually written inside the staging buffer
	uint32_t* stagingIndexPtr = static_cast<uint32_t*>(stagingBufferIndex.getMappedMemory());

	uint32_t offsetVertex = 0U;		// careful: this is a bytes offset
	uint32_t offsetIndex = 0U;		// careful: this is an element (uints) offset
	for (std::vector<Vertex> const& worldVertexes : vertices) {
		// some chunks might be empty, skip them
		if ( worldVertexes.data() == nullptr )
			continue;
		uint32_t sizeData = worldVertexes.size() * vertexSize;
		// insert vertexes of this chunk in staging buffer
		stagingBufferVertex.writeToBuffer(static_cast<const void*>(worldVertexes.data()), sizeData, offsetVertex);
		uint32_t nVoxels = worldVertexes.size() / VERTEX_PER_VOXEL;
		// for every voxel load its face indexes, offsetIndex represents all the vertexes already inserted
		for (uint32_t i = 0; i<nVoxels; i++) {
			for (uint32_t index : indexesVoxel)
			{
				*stagingIndexPtr = index + offsetIndex;
				stagingIndexPtr++;
			}
			offsetIndex += VERTEX_PER_VOXEL;
		}
		offsetVertex += sizeData;
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

void	VulkanModel::setObjectCenter() noexcept
{
	this->boundingCenter = (this->boundingBox.min + this->boundingBox.max) / 2.0f;
}

vec3	VulkanModel::calculateVertexCenter(const std::vector<Vertex>& vertices) noexcept
{
	vec3	center{};

	for (const Vertex& vertex : vertices)
	{
		center += vertex.pos;
	}
	center /= static_cast<float>(vertices.size());
	return center;
}

MeshLayoutDescription	VulkanModel::getModelLayout(uint32_t binding, MeshLayout type) noexcept
{
	MeshLayoutDescription data{};
	data.bindingConfig.resize(1);

	data.bindingConfig[0].binding = binding;
	data.bindingConfig[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	uint32_t locationIndex = 0U, offset = 0U;
	if (type & MeshLayout::VERTEX)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (type & MeshLayout::NORMAL)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (type & MeshLayout::TEXTURE)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32_SFLOAT, offset}
		);
		offset += sizeof(vec2);
	}
	if (type & MeshLayout::RANDOM_INDEX_TEXT)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32_UINT, offset}
		);
		offset += sizeof(uint32_t);
	}
	data.bindingConfig[0].stride = offset;
	return data;
}


void	VulkanModel::Builder::emptyData( void ) noexcept {
	this->vertices.clear();
	this->indices.clear();
}

void	VulkanModel::Builder::loadModel(const std::string &filepath)
{
	std::vector<ObjInfo>		objs = parseOBJFile(filepath);
	std::unordered_map<Vertex, uint32_t>	uniqueVertices{};
	uint32_t 								currentIndex = 0U;

	vertices.clear();
	indices.clear();

	for (const ObjInfo& obj : objs)
	{
		for (const ObjComponent& component : obj.components)
		{
			for (size_t i = 0; i < component.faceIndices.size(); i++)
			{
				const std::vector<uint32_t>& face = component.faceIndices[i];
				const std::vector<uint32_t>& tex = component.textureIndices[i];
				const std::vector<uint32_t>& norm = component.normalIndices[i];

				if (face.size() < 3)
				{
					continue;
				}

				/*	If more than 3 faces, create triangles	*/

				for (size_t j = 2; j < face.size(); j++)
				{
					for (size_t ti : {0UL, j - 1, j})
					{
						Vertex	vertex{};

						vertex.pos = obj.vertices[face[ti]];
						if (tex.size() > ti)
						{
							vertex.textureUv = obj.textureCoords[tex[ti]];
						}
						else
						{
							vec3 absPos = vec3{std::abs(vertex.pos.x), std::abs(vertex.pos.y), std::abs(vertex.pos.z)};
							vec3 norm = vertex.normal;
							
							if (std::abs(norm.x) > 0.5f)
							{
								vertex.textureUv = vec2{(vertex.pos.z + 1.0f) * 0.5f, (vertex.pos. y + 1.0f) * 0.5f};
							}
							else if (std::abs(norm.y) > 0.5f)
							{
								vertex.textureUv = vec2{(vertex.pos.x + 1.0f) * 0.5f, (vertex.pos.z + 1.0f) * 0.5f};
							}
							else
							{
								vertex.textureUv = vec2{(vertex.pos.x + 1.0f) * 0.5f, (vertex.pos.y + 1.0f) * 0.5f};
							}
						}
						if (norm.size() > ti)
						{
							vertex.normal = obj.normals[norm[ti]];
						}
						// vertex.color = generateRandomColor();
						if (uniqueVertices.find(vertex) == uniqueVertices.end()) {
							uniqueVertices[vertex] = currentIndex;
							// new vertex, add it and its vertex index
							this->vertices.push_back(vertex);
							this->indices.push_back(currentIndex++);
						} else {
							// there's already such vertex, add only the vertex index
							this->indices.push_back(uniqueVertices[vertex]);
						}
					}
				}
			}
		}
	}
}

}	// namespace ve
