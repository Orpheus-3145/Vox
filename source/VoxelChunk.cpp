#include <cassert>
#include <cstring>
#include <iostream>

#include "VoxelChunk.hpp"
#include "Config.hpp"
#include "World.hpp"


namespace vox {

vec3i	VoxelChunk::chunkDimensions = vec3i::zero();
vec3i	VoxelChunk::paddedDimensions = vec3i::zero();
ui32	VoxelChunk::paddedSize = 0;
ui32	VoxelChunk::chunkSize = 0;

VoxelChunk::VoxelChunk(vec2i loc, VertexVector* terrainVertexes, VertexVector* undergroundVertexes) :
	location(loc),
	generator{Config::worldSeed, Config::minimumViewingDistance * 2},
	terrainVertexes(terrainVertexes),
	undergroundVertexes(undergroundVertexes)
{
	map.assign(static_cast<size_t>(paddedSize), VoxelType::Padding);
	terrainVertexes->reserve(chunkDimensions.x * chunkDimensions.z * 8);
	undergroundVertexes->reserve(chunkDimensions.x * chunkDimensions.z * 8);

	worldPosition = vec3i(chunkDimensions.x * location.width, 0, chunkDimensions.z * location.depth);
}

void	VoxelChunk::setLocation(vec2i loc)
{
	location = loc;
	worldPosition.x = chunkDimensions.x * loc.width;
	worldPosition.y = 0;
	worldPosition.z = chunkDimensions.z * loc.depth;
}

void	VoxelChunk::generateMap(void)
{
	i32 y;

	const i32 dimX = paddedDimensions.x - 1;
	const i32 dimY = paddedDimensions.y - 1;
	const i32 dimZ = paddedDimensions.z - 1;
	const i32 height = chunkDimensions.y;

	std::fill(map.begin(), map.end(), VoxelType::Padding);

	for (i32 z = 1; z < dimZ; z++)
	{
		for (i32 x = 1; x < dimX; x++)
		{
			i32 index = this->index(x, 1, z);

			const float worldX = static_cast<float>(worldPosition.width + x - 1);
			const float worldZ = static_cast<float>(worldPosition.depth + z - 1);
			float perlinTerrain = this->generator.octavePerlin2D(worldX, worldZ);

			i32 heightValue = static_cast<i32>(perlinTerrain * static_cast<float>(height)) - 3;

			assert(heightValue <= chunkDimensions.height && "height value out of range");

			map[index] = VoxelType::Dirt;
			index++;
			for (y = 2; y < heightValue; y++)
			{
				float perlinCave = this->generator.octavePerlin3D(worldX, static_cast<float>(y), worldZ);
				float t = static_cast<float>(y) / static_cast<float>(height);
				float factor = t * t * (3 - 2 * t);
				float treshold = 0.575f + 0.2f * factor;
				if (perlinCave > treshold)	// create cave
				{
					map[index] = VoxelType::Air;
				}
				else
				{
					if (y > heightValue - 4)
					{
						map[index] = VoxelType::Dirt;
					}
					else
					{
						map[index] = VoxelType::Stone;
					}
				}
				index++;
			}
			for (; y < dimY; y++)
			{
				map[index] = VoxelType::Air;
				index++;
			}
		}
	}
}

void	VoxelChunk::copyAdjacentData()
{
	const VoxelChunk* north = adjacentChunks[static_cast<size_t>(Direction::North)];
	const VoxelChunk* east = adjacentChunks[static_cast<size_t>(Direction::East)];
	const VoxelChunk* south = adjacentChunks[static_cast<size_t>(Direction::South)];
	const VoxelChunk* west = adjacentChunks[static_cast<size_t>(Direction::West)];

	const i32 width = paddedDimensions.x - 1;
	const i32 depth = paddedDimensions.z - 1;
	const i32 copySize = chunkDimensions.y * sizeof(VoxelType);

	if (north != nullptr)
	{
		for (i32 x = 1; x < width; x++)
		{
			i32 src = north->index(x, 1, 1);
			i32 dst = this->index(x, 1, depth);

			std::memcpy(static_cast<void*>(&map[dst]), north->dataAt(src), copySize);
		}
	}
	if (south != nullptr)
	{
		for (i32 x = 1; x < width; x++)
		{
			i32 src = south->index(x, 1, depth - 1);
			i32 dst = this->index(x, 1, 0);

			std::memcpy(&map[dst], south->dataAt(src), copySize);
		}
	}
	if (east != nullptr)
	{
		for (i32 z = 1; z < depth; z++)
		{
			i32 src = east->index(1, 1, z);
			i32 dst = this->index(width, 1, z);

			std::memcpy(&map[dst], east->dataAt(src), copySize);
		}
	}
	if (west != nullptr)
	{
		for (i32 z = 1; z < depth; z++)
		{
			i32 src = west->index(width - 1, 1, z);
			i32 dst = this->index(0, 1, z);

			std::memcpy(&map[dst], west->dataAt(src), copySize);
		}
	}
}

void	VoxelChunk::setAdjacentChunks(VoxelChunk* north, VoxelChunk* east, VoxelChunk* south, VoxelChunk* west) noexcept
{
	adjacentChunks[static_cast<size_t>(Direction::North)] = north;
	adjacentChunks[static_cast<size_t>(Direction::East)] = east;
	adjacentChunks[static_cast<size_t>(Direction::South)] = south;
	adjacentChunks[static_cast<size_t>(Direction::West)] = west;
}

void	VoxelChunk::generateVertexes()
{
	terrainVertexes->clear();
	undergroundVertexes->clear();

	const i32 widthMax = paddedDimensions.x - 1;
	const i32 dimY = paddedDimensions.y - 1;
	const i32 depthMax = paddedDimensions.z - 1;

	const i32 xStride = paddedDimensions.y;
	const i32 zStride = paddedDimensions.x * paddedDimensions.y;

	const float worldX = static_cast<float>(worldPosition.x);
	const float worldZ = static_cast<float>(worldPosition.z);

	copyAdjacentData();
	for (i32 z = 1; z < depthMax; z++)
	{
		for (i32 x = 1; x < widthMax; x++)
		{
			i32 i = index(x, 1, z);
			for (i32 y = 1; y < dimY; y++, i++)
			{
				if (map[i] == VoxelType::Air)
				{
					continue;
				}

				vec3 world{worldX + static_cast<float>(x - 1), static_cast<float>(y - 1), worldZ + static_cast<float>(z - 1)};

				uint32_t textureIndex = ve::randomUint(0U, 1U);
				if (map[i + zStride] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::FRONT), i, textureIndex);
				}
				if (map[i - zStride] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::BACK), i, textureIndex);
				}
				if (map[i - xStride] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::LEFT), i, textureIndex);
				}
				if (map[i + xStride] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::RIGHT), i, textureIndex);
				}
				if (map[i + 1] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::TOP), i, textureIndex);
				}
				if (map[i - 1] == VoxelType::Air)
				{
					addVoxelFace(world, static_cast<size_t>(VertexFaces::BOTTOM), i, textureIndex);
				}
			}
		}
	}
}

void	VoxelChunk::addVoxelFace(const vec3& location, size_t min, i32 voxelIndex, uint32_t textureIndex)
{
	size_t max = min + 4;

	for (size_t i = min; i < max; i++)
	{
		switch (map[voxelIndex])
		{
			case VoxelType::Dirt:
				if (textureIndex == 0)
				{
					terrainVertexes->emplace_back
					(
						ve::VulkanModel::Vertex
						{
							vec3
							{
								VOXEL_VERTEXES_ATLAS[i].pos.x + location.x,
								VOXEL_VERTEXES_ATLAS[i].pos.y + location.y,
								VOXEL_VERTEXES_ATLAS[i].pos.z + location.z
							},
						VOXEL_VERTEXES_ATLAS[i].normal,
						VOXEL_VERTEXES_ATLAS[i].textureUv,
						textureIndex
					});
				}
				else
				{
					terrainVertexes->emplace_back
					(
						ve::VulkanModel::Vertex
						{
							vec3
							{
								VOXEL_VERTEXES[i].pos.x + location.x,
								VOXEL_VERTEXES[i].pos.y + location.y,
								VOXEL_VERTEXES[i].pos.z + location.z
							},
						VOXEL_VERTEXES[i].normal,
						VOXEL_VERTEXES[i].textureUv,
						textureIndex
					});

				}
				break;

			case VoxelType::Stone:
				undergroundVertexes->emplace_back
				(
					ve::VulkanModel::Vertex
					{
						vec3
						{
							VOXEL_VERTEXES[i].pos.x + location.x,
							VOXEL_VERTEXES[i].pos.y + location.y,
							VOXEL_VERTEXES[i].pos.z + location.z
						},
					VOXEL_VERTEXES[i].normal,
					VOXEL_VERTEXES[i].textureUv,
					textureIndex
				});
				break;

			default:
				break;
		}
	}
}

bool	VoxelChunk::testForCollision(const std::vector<vec3i>& locations) const noexcept
{
	for (const vec3i& loc : locations)
	{
		if (loc.x < -1 || loc.x >= paddedDimensions.x ||
			loc.y < 0 || loc.y >= paddedDimensions.y ||
			loc.z < -1 || loc.z >= paddedDimensions.z)
		{
			continue;
		}
		const VoxelType voxel = this->at(loc.x + 1, loc.y + 1, loc.z + 1);
		if (voxel != VoxelType::Air && voxel != VoxelType::Padding)
		{
			return true;
		}
	}
	return false;
}

}	// namespace vox
