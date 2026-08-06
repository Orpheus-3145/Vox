#pragma once

#include <array>
#include <memory>

#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"


namespace ve {

enum class MeshLayout : uint32_t		// NB change name
{
	VERTEX = 1 << 0,
	NORMAL = 1 << 1,
	TEXTURE = 1 << 2,
	RANDOM_INDEX_TEXT = 1 << 3
};

constexpr MeshLayout operator|(MeshLayout a, MeshLayout b) {
	return static_cast<MeshLayout>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
	);
}

constexpr bool operator&(MeshLayout a, MeshLayout b) {
	return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

enum class MeshType : uint8_t
{
	VOXEL,
	FACE
};

constexpr inline MeshLayout DEFAULT_MODEL_LAYOUT = MeshLayout::VERTEX | MeshLayout::NORMAL | MeshLayout::TEXTURE | MeshLayout::RANDOM_INDEX_TEXT;
constexpr inline MeshLayout ONLY_VERTEX_LAYOUT = MeshLayout::VERTEX;
constexpr inline MeshLayout FONT_MODEL_LAYOUT = MeshLayout::VERTEX | MeshLayout::TEXTURE;

struct Vertex
{
	vec3		pos{0.0f};
	vec3		normal{0.0f};
	vec2		textureUv{0.0f};
	uint32_t	textureIndex{0U};		// NB remove it

	constexpr Vertex( vec3 const& pos, vec3 const& normal, vec2 const& textureUv, uint32_t textureIndex ) :
		pos{pos},
		normal{normal},
		textureUv{textureUv},
		textureIndex{textureIndex} {};
    constexpr Vertex( void ) = default;

	bool operator==(Vertex const& other) const noexcept
	{
		return	pos == other.pos &&
				normal == other.normal &&
				textureUv == other.textureUv &&
				textureIndex == other.textureIndex;
	}

	bool operator!=(Vertex const& other) const noexcept
	{
		return !(*this == other);
	}

	bool operator<(Vertex const& other) const noexcept
	{
		if (pos != other.pos)
			return pos < other.pos;
		if (normal != other.normal)
			return normal < other.normal;
		return textureUv < other.textureUv;
	}
};

using VertexVector = std::vector<Vertex>;
using IndexVector = std::vector<ui32>;

struct MeshLayoutDescription
{
	std::vector<VkVertexInputBindingDescription>	bindingConfig;
	std::vector<VkVertexInputAttributeDescription>	attributeConfig;
};

class VulkanModel
{
	public:
		VulkanModel() = delete;
		VulkanModel(VulkanDevice& device, VertexVector const& vertices, IndexVector const& indices, uint32_t binding = 0U, MeshLayout layout = DEFAULT_MODEL_LAYOUT);
		VulkanModel(VulkanDevice& device, std::vector<VertexVector*> const& vertices, IndexVector const& instanceIndices, size_t nInstances, uint32_t binding = 0U, MeshLayout layout = DEFAULT_MODEL_LAYOUT);
		~VulkanModel(void) noexcept = default;

		VulkanModel(VulkanModel const&) = delete;
		VulkanModel(VulkanModel&&) = default;
		VulkanModel& operator=(VulkanModel const&) = delete;
		VulkanModel& operator=(VulkanModel&&) = delete;
		
		void	bindBuffer(VkCommandBuffer commandBuffer) const noexcept;
		void	draw(VkCommandBuffer commandBuffer) const noexcept;
		
		static MeshLayoutDescription	getModelLayout(uint32_t binding = 0U, MeshLayout type = DEFAULT_MODEL_LAYOUT) noexcept;

	private:
		VulkanDevice&	vulkanDevice;
		uint32_t		binding;
		MeshLayout		layout;
		bool			isIndexed{false};

		size_t			vertexCount{0UL};
		size_t			indexCount{0UL};

		std::unique_ptr<VulkanBuffer>	vertexBuffer;
		std::unique_ptr<VulkanBuffer>	indexBuffer;

		void	createVertexBuffer(VertexVector const& vertices);
		void	createIndexBuffer(IndexVector const& indices);
		void	createVertexIndexBuffer(std::vector<VertexVector*> const& vertexes, IndexVector const& instanceIndices, size_t nInstances);
};

}	// namespace ve
