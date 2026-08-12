#pragma once

#include <array>
#include <memory>

#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"


namespace ve {

enum class VertexLayout : uint32_t
{
	VERTEX = 1 << 0,
	NORMAL = 1 << 1,
	TEXTURE = 1 << 2,
};

constexpr VertexLayout operator|(VertexLayout a, VertexLayout b) {
	return static_cast<VertexLayout>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
	);
}

constexpr bool operator&(VertexLayout a, VertexLayout b) {
	return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

constexpr inline VertexLayout DEFAULT_MODEL_LAYOUT = VertexLayout::VERTEX | VertexLayout::NORMAL | VertexLayout::TEXTURE;
constexpr inline VertexLayout ONLY_VERTEX_LAYOUT = VertexLayout::VERTEX;
constexpr inline VertexLayout FONT_MODEL_LAYOUT = VertexLayout::VERTEX | VertexLayout::TEXTURE;

struct Vertex
{
	vec3		pos{0.0f};
	vec3		normal{0.0f};
	vec2		textureUv{0.0f};

	constexpr Vertex( vec3 const& pos, vec3 const& normal, vec2 const& textureUv ) :
		pos{pos},
		normal{normal},
		textureUv{textureUv} {};
    constexpr Vertex( void ) = default;

	bool operator==(Vertex const& other) const noexcept
	{
		return	pos == other.pos &&
				normal == other.normal &&
				textureUv == other.textureUv;
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
		VulkanModel(VulkanDevice& device, VertexVector const& vertices, IndexVector const& indices, uint32_t binding = 0U, VertexLayout layout = DEFAULT_MODEL_LAYOUT);
		VulkanModel(VulkanDevice& device, VertexVector const& vertices, IndexVector const& instanceIndices, size_t nInstances, uint32_t binding = 0U, VertexLayout layout = DEFAULT_MODEL_LAYOUT);
		VulkanModel(VulkanDevice& device, std::vector<VertexVector*> const& vertices, IndexVector const& instanceIndices, size_t nInstances, uint32_t binding = 0U, VertexLayout layout = DEFAULT_MODEL_LAYOUT);
		~VulkanModel(void) noexcept = default;

		VulkanModel(VulkanModel const&) = delete;
		VulkanModel(VulkanModel&&) = default;
		VulkanModel& operator=(VulkanModel const&) = delete;
		VulkanModel& operator=(VulkanModel&&) = delete;
		
		void	bindBuffer(VkCommandBuffer commandBuffer) const noexcept;
		void	draw(VkCommandBuffer commandBuffer) const noexcept;

		static MeshLayoutDescription	getModelLayout(uint32_t binding = 0U, VertexLayout type = DEFAULT_MODEL_LAYOUT) noexcept;
		VkDeviceSize					getBufferSize(void) const noexcept;

	private:
		VulkanDevice&	vulkanDevice;
		uint32_t		binding;
		VertexLayout	layout;
		bool			isIndexed{false};

		size_t			vertexCount{0UL};
		size_t			indexCount{0UL};

		std::unique_ptr<VulkanBuffer>	vertexBuffer;
		std::unique_ptr<VulkanBuffer>	indexBuffer;

		void	createVertexBuffer(VertexVector const& vertices);
		void	createIndexBuffer(IndexVector const& indices);
		void	createVertexIndexBuffer(VertexVector const& vertexes, IndexVector const& instanceIndices, size_t nInstances);
		void	createVertexIndexBuffer(std::vector<VertexVector*> const& vertexes, IndexVector const& instanceIndices, size_t nInstances);
};

}	// namespace ve
