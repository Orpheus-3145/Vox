#include "VoxelMap.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "World.hpp"

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <functional>


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
	playerOnChunk = voxelToChunkPosition(Config::startingPosition);
	minPositions = vec2i{playerOnChunk.x - (squareSize - 1) / 2, playerOnChunk.y - (squareSize - 1) / 2};
	maxPositions = vec2i{minPositions.x + squareSize - 1, minPositions.y + squareSize - 1};

	std::cout << "Map ranges from: " << minPositions << " to: " << maxPositions << std::endl;
	playerOnChunk = vec2i{minPositions.x + squareSize / 2, minPositions.y + squareSize / 2};
	VoxelChunk::paddedDimensions = VoxelChunk::chunkDimensions + vec3i{2, 2, 2};
	scheduledChanges.resize(visibleChunks);
}

void	VoxelMap::regenerateTerrainBuffer()
{
	size_t	totalVertexes = 0;
	size_t	oldVertexSize = terrainVertexes.size();
	
	terrainVertexes.clear();
	for (size_t i = 0; i < map.size(); i++)
	{
		totalVertexes += map[i].getVertexTerrainSize();
	}
	if (totalVertexes > oldVertexSize)
	{
		terrainVertexes.reserve(totalVertexes);
		terrainIndexes.reserve(totalVertexes * 6 / 4);

		for (ui32 i = oldVertexSize; i < static_cast<ui32>(totalVertexes); i += 4)
		{
			IndexVector indexes = {0U + i, 1U + i, 2U + i, 0U + i, 2U + i, 3U + i};
			terrainIndexes.insert(terrainIndexes.end(), indexes.begin(), indexes.end());
		}
	}
	else
	{
		terrainIndexes.erase(terrainIndexes.begin() + totalVertexes * 6 / 4, terrainIndexes.end());
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		const VertexVector& chunkVertexes = map[i].getVertexTerrainData();

		terrainVertexes.insert(terrainVertexes.end(), chunkVertexes.begin(), chunkVertexes.end());
	}
}

void	VoxelMap::regenerateUndergroundBuffer()
{
	size_t	totalVertexes = 0;
	size_t	oldVertexSize = undergroundVertexes.size();
	
	undergroundVertexes.clear();
	for (size_t i = 0; i < map.size(); i++)
	{
		totalVertexes += map[i].getVertexUndergroundSize();
	}
	if (totalVertexes > oldVertexSize)
	{
		undergroundVertexes.reserve(totalVertexes);
		undergroundIndexes.reserve(totalVertexes * 6 / 4);

		for (ui32 i = oldVertexSize; i < static_cast<ui32>(totalVertexes); i += 4)
		{
			IndexVector indexes = {0U + i, 1U + i, 2U + i, 0U + i, 2U + i, 3U + i};
			undergroundIndexes.insert(undergroundIndexes.end(), indexes.begin(), indexes.end());
		}
	}
	else
	{
		undergroundIndexes.erase(undergroundIndexes.begin() + totalVertexes * 6 / 4, undergroundIndexes.end());
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		const VertexVector& chunkVertexes = map[i].getVertexUndergroundData();

		undergroundVertexes.insert(undergroundVertexes.end(), chunkVertexes.begin(), chunkVertexes.end());
	}
}

std::unique_ptr<ve::VulkanModel> VoxelMap::createNewTerrainModel(ve::VulkanDevice& device, ui32 binding)
{
	return std::make_unique<ve::VulkanModel>(device, terrainVertexes, terrainIndexes, binding);
}

//	what happens if no underground by chance?
std::unique_ptr<ve::VulkanModel> VoxelMap::createNewUndergroundModel(ve::VulkanDevice& device, ui32 binding)
{
	return std::make_unique<ve::VulkanModel>(device, undergroundVertexes, undergroundIndexes, binding);
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
			VoxelChunk chunk(vec2i(minPositions.x + x, minPositions.y + z));
			map.emplace_back(std::move(chunk));
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
	regenerateTerrainBuffer();
	regenerateUndergroundBuffer();
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
