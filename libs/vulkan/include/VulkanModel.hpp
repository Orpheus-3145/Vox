#pragma once

#include <array>
#include <memory>

#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"


namespace ve {

inline constexpr uint32_t	VERTEX_PER_VOXEL = 24U;	// number of vertexes per voxel		NB move inside World.hpp
inline constexpr uint32_t	INDEX_PER_VOXEL = 36U;	// number of vertex indexes per voxel
inline constexpr uint32_t	VERTEX_PER_FACE = 4U;	// number of vertexes per face (of a voxel)
inline constexpr uint32_t	INDEX_PER_FACE = 6U;	// number of vertex indexes per voxel

// hard-coded indexes of a voxel
inline constexpr std::array<ui32, ve::INDEX_PER_VOXEL> VOXEL_INDEXES{		// NB move to world.hpp
	0U, 2U, 1U, 		// front face
	0U, 3U, 2U, 		// front face
	4U, 6U, 5U, 		// back face
	4U, 7U, 6U, 		// back face
	8U, 10U, 9U, 		// left face
	8U, 11U, 10U, 		// left face
	12U, 14U, 13U, 		// right face
	12U, 15U, 14U, 		// right face
	16U, 18U, 17U, 		// top face
	16U, 19U, 18U, 		// top face
	20U, 22U, 21U, 		// bottom face
	20U, 23U, 22U		// bottom face
};

// hard-coded indexes of a face
inline constexpr std::array<ui32, ve::INDEX_PER_FACE> FACE_INDEXES{
	0U, 2U, 1U, 		// front face
	0U, 3U, 2U	 		// front face
};

enum class MeshLayout : uint32_t
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


struct MeshLayoutDescription
{
	std::vector<VkVertexInputBindingDescription>	bindingConfig;
	std::vector<VkVertexInputAttributeDescription>	attributeConfig;
};


class VulkanModel
{
	public:
		struct Vertex
		{
			vec3		pos{0.0f};
			vec3		normal{0.0f};
			vec2		textureUv{0.0f};
			uint32_t	textureIndex{0U};			// NB remove it

			constexpr Vertex( vec3 const& pos, vec3 const& normal, vec2 const& textureUv, uint32_t textureIndex ) :
				pos{pos},
				normal{normal},
				textureUv{textureUv},
				textureIndex{textureIndex} {};
			constexpr Vertex( void ) = default;

			bool operator==(const Vertex& other) const noexcept
			{
				return	pos == other.pos &&
						normal == other.normal &&
						textureUv == other.textureUv &&
						textureIndex == other.textureIndex;
			}
			bool operator!=(const Vertex& other) const noexcept
			{
				return !(*this == other);
			}
			bool operator<(const Vertex& other) const noexcept
			{
				if (pos != other.pos)
					return pos < other.pos;
				if (normal != other.normal)
					return normal < other.normal;
				return textureUv < other.textureUv;
			}
		};

		VulkanModel() = delete;
		VulkanModel(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t binding = 0U, MeshLayout layout = DEFAULT_MODEL_LAYOUT);
		VulkanModel(VulkanDevice& device, const std::vector<std::vector<Vertex>*>& vertices, MeshType type, uint32_t binding = 0U, MeshLayout layout = DEFAULT_MODEL_LAYOUT);
		~VulkanModel(void) noexcept = default;

		VulkanModel(const VulkanModel&) = delete;
		VulkanModel(VulkanModel&&) = default;
		VulkanModel& operator=(const VulkanModel&) = delete;
		VulkanModel& operator=(VulkanModel&&) = delete;
		
		void	bindBuffer(VkCommandBuffer commandBuffer) const noexcept;
		void	draw(VkCommandBuffer commandBuffer) const noexcept;
		
		static MeshLayoutDescription	getModelLayout(uint32_t binding = 0U, MeshLayout type = DEFAULT_MODEL_LAYOUT) noexcept;

	private:
		VulkanDevice&	vulkanDevice;
		uint32_t		binding;
		MeshLayout		layout;
		bool			isIndexed{false};

		uint32_t		vertexCount{0U};
		uint32_t		indexCount{0U};

		std::unique_ptr<VulkanBuffer>	vertexBuffer;
		std::unique_ptr<VulkanBuffer>	indexBuffer;

		void	createVertexBuffer(const std::vector<Vertex>& vertices);
		void	createIndexBuffer(const std::vector<uint32_t>& indices);
		void	createVertexIndexBuffer(const std::vector<std::vector<Vertex>*>& vertexes, MeshType type);
};

}	// namespace ve
