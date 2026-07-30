#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <functional>

#include "VoxelMap.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "World.hpp"


namespace vox {

VoxelMap::VoxelMap(ThreadManager& threadManager) :
	threadManager(threadManager)
{
	i32 visibleVoxels = static_cast<i32>(Config::minimumViewingDistance * 2);
	this->squareSize = visibleVoxels / static_cast<i32>(Config::chunkLength) + 1;
	i32 visibleChunks = this->squareSize * this->squareSize;

	VoxelChunk::chunkDimensions = vec3i{Config::chunkLength, Config::chunkHeight, Config::chunkLength};
	VoxelChunk::chunkSize = Config::chunkLength * Config::chunkHeight * Config::chunkLength;
	VoxelChunk::paddedSize = (Config::chunkLength + 2) * (Config::chunkHeight + 2) * (Config::chunkLength + 2);

	map.reserve(visibleChunks);
	terrainVertexes.resize(visibleChunks);
	undergroundVertexes.resize(visibleChunks);

	playerOnChunk = voxelToChunkPosition(Config::startingPosition);
	minPositions = vec2i{playerOnChunk.x - (squareSize - 1) / 2, playerOnChunk.y - (squareSize - 1) / 2};
	maxPositions = vec2i{minPositions.x + squareSize - 1, minPositions.y + squareSize - 1};

	std::cout << "Map ranges from: " << minPositions << " to: " << maxPositions << std::endl;
	playerOnChunk = vec2i{minPositions.x + squareSize / 2, minPositions.y + squareSize / 2};
	VoxelChunk::paddedDimensions = VoxelChunk::chunkDimensions + vec3i{2, 2, 2};
	scheduledChanges.resize(visibleChunks);
}

std::unique_ptr<ve::VulkanModel> VoxelMap::createNewTerrainModel(ve::VulkanDevice& device, ui32 binding)
{
	return std::make_unique<ve::VulkanModel>(device, terrainVertexes, ve::MeshType::FACE, binding);
}

// NB what happens if no underground by chance?
std::unique_ptr<ve::VulkanModel> VoxelMap::createNewUndergroundModel(ve::VulkanDevice& device, ui32 binding)
{
	return std::make_unique<ve::VulkanModel>(device, undergroundVertexes, ve::MeshType::FACE, binding);
}

void	VoxelMap::setAdjacentPointers()
{
	i32 index = 0;
	for (i32 depth = 0; depth < squareSize; depth++)
	{
		for (i32 width = 0; width < squareSize; width++)
		{
			VoxelChunk* north;
			VoxelChunk* east;
			VoxelChunk* south;
			VoxelChunk* west;

			if (depth == 0)
			{
				south = nullptr;
			}
			else
			{
				south = &map[index - squareSize];
			}
			if (depth == squareSize - 1)
			{
				north = nullptr;
			}
			else
			{
				north = &map[index + squareSize];
			}
			if (width == 0)
			{
				west = nullptr;
			}
			else
			{
				west = &map[index - 1];
			}
			if (width == squareSize - 1)
			{
				east = nullptr;
			}
			else
			{
				east = &map[index + 1];
			}
			map[index].setAdjacentChunks(north, east, south, west);
			index++;
		}
	}
}

void	VoxelMap::init()
{
	Stopwatch timer;
	
	timer.start();
	
	for (i32 z = 0; z < squareSize; z++)
	{
		for (i32 x = 0; x < squareSize; x++)
		{
			VertexVector*	chunkTerrainVertexes = &terrainVertexes[z * squareSize + x];
			VertexVector*	chunkUndergroundVertexes = &undergroundVertexes[z * squareSize + x];
			map.emplace_back(vec2i(minPositions.x + x, minPositions.y + z), chunkTerrainVertexes, chunkUndergroundVertexes);
		}
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		threadManager.enqueue([this, i] {
			map[i].generateMap();
		});
	}
	setAdjacentPointers();
	threadManager.waitIdle();
	timer.stop();
	std::cout << "Initial chunk generation complete in: " << timer << std::endl;
	timer.reset();
	timer.start();
	for (size_t i = 0; i < map.size(); i++)
	{
		threadManager.enqueue([this, i] {
			map[i].generateVertexes();
		});
	}
	threadManager.waitIdle();
	timer.stop();
	std::cout << "Initial voxel map generation took: " << timer << std::endl;
}

vec2i	VoxelMap::voxelToChunkPosition(const vec3& position) const noexcept
{
	vec2i	chunkPos{
		static_cast<i32>(std::floor(position.x / static_cast<float>(VoxelChunk::chunkDimensions.x))),
		static_cast<i32>(std::floor(position.z / static_cast<float>(VoxelChunk::chunkDimensions.z)))
	};
	return chunkPos;
}

vec3	VoxelMap::getMapMiddle() const noexcept
{
	return vec3((maxPositions.x + minPositions.x + 1) * VoxelChunk::chunkDimensions.x / 2.0f,
				VoxelChunk::chunkDimensions.height - 1.0f,
				(maxPositions.y + minPositions.y + 1) * VoxelChunk::chunkDimensions.z / 2.0f);
}

}	// namespace vox
