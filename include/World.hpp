#pragma once

#include "ThreadManager.hpp"
#include "Vulkan.hpp"
#include "Vectors.hpp"
#include "Stopwatch.hpp"
#include "VoxelMap.hpp"
#include "Config.hpp"
#include "TypeAliases.hpp"

#include <unordered_map>
#include <array>


namespace vox {

inline constexpr ui32	VERTEX_PER_VOXEL = 24U;	// number of vertexes per voxel
inline constexpr ui32	INDEX_PER_VOXEL = 36U;	// number of vertex indexes per voxel
inline constexpr float		VOXEL_SIZE = 1.0f;		// length of a voxel edge

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
inline constexpr std::array<ve::VulkanModel::Vertex,VERTEX_PER_VOXEL> VOXEL_VERTEXES{
	// face FRONT (z = +0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::forward(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::forward(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::forward(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::forward(), vec2{ 0.0f, 0.0f }},
	//face BACK (z = -0.5)
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::backward(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::backward(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::backward(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::backward(), vec2{ 0.0f, 0.0f }},
	// face LEFT (x = -0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::left(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::left(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::left(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::left(), vec2{ 0.0f, 0.0f }},
	// face RIGHT (x = +0.5)
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::right(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::right(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::right(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::right(), vec2{ 0.0f, 0.0f }},
	// face TOP (y = +0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::up(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::up(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::up(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::up(), vec2{ 0.0f, 0.0f }},
	// face BOTTOM (y = -0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::down(), vec2{ 0.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::down(), vec2{ 1.0f, 1.0f }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::down(), vec2{ 1.0f, 0.0f }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::down(), vec2{ 0.0f, 0.0f }}
};


static constexpr float W = 1.0f / 4.0f;  // width of a tile
static constexpr float H = 1.0f / 3.0f;  // height of a tile
static constexpr float padding = 0.004f;
// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (atlas texture coordinates)
inline constexpr std::array<ve::VulkanModel::Vertex,VERTEX_PER_VOXEL> VOXEL_VERTEXES_ATLAS{
	// face FRONT (z = +0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::forward(), vec2{ W + padding, 3 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::forward(), vec2{ 2 * W - padding, 3 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::forward(), vec2{ 2 * W - padding, 2 * H + padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::forward(), vec2{ W + padding, 2 * H + padding }},
	//face BACK (z = -0.5)
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::backward(), vec2{ 2 * W - padding, padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::backward(), vec2{ W + padding, padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::backward(), vec2{ W + padding, H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::backward(), vec2{ 2 * W - padding, H - padding }},
	// face LEFT (x = -0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::left(), vec2{ padding, H + padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::left(), vec2{ padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::left(), vec2{ W - padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::left(), vec2{ W - padding, H + padding }},
	// face RIGHT (x = +0.5)
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::right(), vec2{ 3 * W - padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::right(), vec2{ 3 * W - padding, H + padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::right(), vec2{ 2 * W + padding, H + padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::right(), vec2{ 2 * W + padding, 2 * H - padding }},
	// face TOP (y = +0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f,  0.5f }, vec3::up(), vec2{ W + padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f,  0.5f }, vec3::up(), vec2{ 2 * W - padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f,  0.5f, -0.5f }, vec3::up(), vec2{ 2 * W - padding, H + padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f,  0.5f, -0.5f }, vec3::up(), vec2{ W + padding, H + padding }},
	// face BOTTOM (y = -0.5)
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f, -0.5f }, vec3::down(), vec2{ 3 * W + padding, H + padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f, -0.5f }, vec3::down(), vec2{ 3 * W + padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{  0.5f, -0.5f,  0.5f }, vec3::down(), vec2{ 4 * W - padding, 2 * H - padding }},
	ve::VulkanModel::Vertex{vec3{ -0.5f, -0.5f,  0.5f }, vec3::down(), vec2{ 4 * W - padding, H + padding }}
};

// hard-coded face indexes of a voxel
inline constexpr std::array<ui32, INDEX_PER_VOXEL> VOXEL_VERTEX_INDEXES{
	0U, 1U, 2U, 		// front face
	0U, 2U, 3U, 		// front face
	4U, 5U, 6U, 		// back face
	4U, 6U, 7U, 		// back face
	8U, 9U, 10U, 		// left face
	8U, 10U, 11U, 		// left face
	12U, 13U, 14U, 		// right face
	12U, 14U, 15U, 		// right face
	16U, 17U, 18U, 		// top face
	16U, 18U, 19U, 		// top face
	20U, 21U, 22U, 		// bottom face
	20U, 22U, 23U		// bottom face
};

std::vector<vec3>						getVertexRelative( vec3 const& relativeOrigin );
// std::vector<ve::VulkanModel::Vertex>	getVertexRelative( vec3 const& = vec3(0.0f) );
std::vector<ve::VulkanModel::Vertex>	getVertexAtlasRelative( vec3 const& = vec3(0.0f) );
IndexVector			getIndexRelative( ui32 = 0U );

}	// namespace vox
