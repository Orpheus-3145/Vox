#pragma once

#include <array>

#include "ThreadManager.hpp"
#include "Vulkan.hpp"
#include "Vectors.hpp"
#include "Stopwatch.hpp"
#include "VoxelMap.hpp"
#include "Config.hpp"
#include "TypeAliases.hpp"


namespace vox {

inline constexpr float	VOXEL_SIZE = 1.0f;		// length of a voxel edge

enum VertexFaces : size_t
{
	FRONT = 0,
	BACK = 4,
	LEFT = 8,
	RIGHT = 12,
	TOP = 16,
	BOTTOM = 20
};

// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (standard texture coordinates)
inline constexpr std::array<ve::VulkanModel::Vertex,ve::VERTEX_PER_VOXEL> VOXEL_VERTEXES{
	// face FRONT (z = 1)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::forward(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::forward(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::forward(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::forward(), vec2{ 0.0f, 0.0f }, 0U},
	//face BACK (z = 0)
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::backward(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::backward(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::backward(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::backward(), vec2{ 0.0f, 0.0f }, 0U},
	// face LEFT (x = 0)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::left(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::left(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::left(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::left(), vec2{ 0.0f, 0.0f }, 0U},
	// face RIGHT (x = 1)
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::right(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::right(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::right(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::right(), vec2{ 0.0f, 0.0f }, 0U},
	// face TOP (y = 1)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::up(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::up(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 0.0f, 0.0f }, 0U},
	// face BOTTOM (y = 0)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 0.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 1.0f, 1.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 1.0f, 0.0f }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 0.0f, 0.0f }, 0U}
};


static constexpr float W = 1.0f / 4.0f;  // width of a tile
static constexpr float H = 1.0f / 3.0f;  // height of a tile
static constexpr float padding = 0.004f;
// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (atlas texture coordinates)
inline constexpr std::array<ve::VulkanModel::Vertex,ve::VERTEX_PER_VOXEL> VOXEL_VERTEXES_ATLAS{
	// face FRONT (z = 1)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::forward(), vec2{ W + padding, 3 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::forward(), vec2{ 2 * W - padding, 3 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::forward(), vec2{ 2 * W - padding, 2 * H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::forward(), vec2{ W + padding, 2 * H + padding }, 0U},
	//face BACK (z = 0)
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::backward(), vec2{ 2 * W - padding, padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::backward(), vec2{ W + padding, padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::backward(), vec2{ W + padding, H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::backward(), vec2{ 2 * W - padding, H - padding }, 0U},
	// face LEFT (x = 0)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::left(), vec2{ padding, H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::left(), vec2{ padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::left(), vec2{ W - padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::left(), vec2{ W - padding, H + padding }, 0U},
	// face RIGHT (x = 1)
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::right(), vec2{ 3 * W - padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::right(), vec2{ 3 * W - padding, H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::right(), vec2{ 2 * W + padding, H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::right(), vec2{ 2 * W + padding, 2 * H - padding }, 0U},
	// face TOP (y = 1)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::up(), vec2{ W + padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::up(), vec2{ 2 * W - padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 2 * W - padding, H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::up(), vec2{ W + padding, H + padding }, 0U},
	// face BOTTOM (y = 0)
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 3 * W + padding, H + padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 3 * W + padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 4 * W - padding, 2 * H - padding }, 0U},
	ve::VulkanModel::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 4 * W - padding, H + padding }, 0U}
};

VertexVector	getVertexRelative( vec3 const& relativeOrigin = vec3(0.0f) );
VertexVector	getVertexAtlasRelative( vec3 const& relativeOrigin = vec3(0.0f) );
IndexVector		getIndexRelative( ui32 = 0U );

std::unique_ptr<ve::VulkanModel>	 		createVoxelModel( ve::VulkanDevice& vulkanDevice, vec3 const& = vec3{-0.5f, -0.5f, -0.5f} );
std::unique_ptr<ve::VulkanModel>	 		createVoxelAtlasModel( ve::VulkanDevice& vulkanDevice, vec3 const& = vec3{-0.5f, -0.5f, -0.5f} );

}	// namespace vox
