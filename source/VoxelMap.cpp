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
	
	this->rawPosition = Config::startingPosition;

	map.reserve(visibleChunks);
	playerOnChunk = voxelToChunkPosition(this->rawPosition);
	minPositions = vec2i{playerOnChunk.x - (squareSize - 1) / 2, playerOnChunk.y - (squareSize - 1) / 2};
	maxPositions = vec2i{minPositions.x + squareSize - 1, minPositions.y + squareSize - 1};

	std::cout << "Map ranges from: " << minPositions << " to: " << maxPositions << std::endl;
	playerOnChunk = vec2i{minPositions.x + squareSize / 2, minPositions.y + squareSize / 2};
	rawPosition = vec3::zero();
	VoxelChunk::paddedDimensions = VoxelChunk::chunkDimensions + vec3i{2, 2, 2};
}

std::unique_ptr<ve::VulkanModel> VoxelMap::createNewModelTerrain( ve::VulkanDevice& device, ui32 binding )
{
	std::unique_ptr<ve::VulkanModel> model;

	model = std::make_unique<ve::VulkanModel>(device, modelVector, modelIndexes, 0U, ve::DEFAULT_MODEL_LAYOUT);
	return model;
}

void	VoxelMap::updateModel()
{
	size_t totalVertexes = 0;

	for (size_t i = 0; i < map.size(); i++)
	{
		totalVertexes += map[i].getVertexTerrainSize();
	}
	if (totalVertexes > modelVector.size())
	{
		const ui32 prevSize = static_cast<ui32>(modelVector.size());

		modelVector.reserve(totalVertexes);
		modelIndexes.reserve(totalVertexes * 6 / 4);

		for (ui32 i = prevSize; i < modelVector.capacity(); i += 4)
		{
			IndexVector indexes = {0U + i, 1U + i, 2U + i, 0U + i, 2U + i, 3U + i};
			modelIndexes.insert(modelIndexes.end(), indexes.begin(), indexes.end());
		}
	}
	else
	{
		modelVector.clear();
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		const VertexVector& chunkVertexes = map[i].getVertexTerrainData();

		modelVector.insert(modelVector.end(), chunkVertexes.begin(), chunkVertexes.end());
	}
	for (ui32 i = 0; i < modelVector.size(); i += 4)
	{
		IndexVector indexes = {0U + i, 1U + i, 2U + i, 0U + i, 2U + i, 3U + i};
		modelIndexes.insert(modelIndexes.end(), indexes.begin(), indexes.end());
	}
	model = std::make_unique<ve::VulkanModel>(device, modelVector, modelIndexes, binding, ve::DEFAULT_MODEL_LAYOUT);
	return model;
}

std::unique_ptr<ve::VulkanModel> VoxelMap::createNewModelUnderground( ve::VulkanDevice& device, ui32 binding )
{
	std::unique_ptr<ve::VulkanModel> model;
	size_t totalVertexes = 0;

	modelVector.clear();
	modelIndexes.clear();
	for (size_t i = 0; i < map.size(); i++)
	{
		totalVertexes += map[i].getVertexUndergroundSize();
	}
	if (totalVertexes > modelVector.capacity())
	{
		modelVector.reserve(totalVertexes);
		modelIndexes.reserve(totalVertexes * 6 / 4);
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		const VertexVector& chunkVertexes = map[i].getVertexUndergroundData();

		modelVector.insert(modelVector.end(), chunkVertexes.begin(), chunkVertexes.end());
	}
	for (ui32 i = 0; i < modelVector.size(); i += 4)
	{
		IndexVector indexes = {0U + i, 1U + i, 2U + i, 0U + i, 2U + i, 3U + i};
		modelIndexes.insert(modelIndexes.end(), indexes.begin(), indexes.end());
	}
	model = std::make_unique<ve::VulkanModel>(device, modelVector, modelIndexes, binding, ve::DEFAULT_MODEL_LAYOUT);
	return model;
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
	setAdjacentPointers();
	for (size_t i = 0; i < map.size(); i++)
	{
		threadManager.enqueue([&] {
			chunk.generateMap();
		});
	}
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
	updateModel();
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

}	// namespace vox
