#pragma once

#include "ThreadManager.hpp"
#include "Vectors.hpp"
#include "VoxelChunk.hpp"
#include "World.hpp"
#include "TypeAliases.hpp"


namespace vox {

enum class Direction : ui8
{
	North,
	East,
	South,
	West
};

class VoxelMap
{
	public:
		VoxelMap() = delete;
		VoxelMap(ThreadManager& threadManager);
		~VoxelMap() = default;
		VoxelMap(VoxelMap const&) = delete;
		VoxelMap(VoxelMap&&) = delete;
		VoxelMap& operator=(VoxelMap const&) = delete;
		VoxelMap& operator=(VoxelMap&&) = delete;

		bool	update(const vec3& newPosition);
		void	init();

		std::unique_ptr<ve::VulkanModel> createNewTerrainModel(ve::VulkanDevice& device, ui32 binding = 0U);
		std::unique_ptr<ve::VulkanModel> createNewUndergroundModel(ve::VulkanDevice& device, ui32 binding = 0U);
		vec3	getMapMiddle() const noexcept;
		void	detectCollision(vec3& movement);
		
		VoxelType	getVoxelAt(const vec3& location);
		
	private:
		std::vector<VoxelChunk>	map;
		
		i32 	squareSize;
		vec2i	minPositions;
		vec2i	maxPositions;
		vec2i	playerOnChunk;
		vec3	rawPosition;
		
		VertexVector	terrainVertexes;
		IndexVector		terrainIndexes;
		
		VertexVector	undergroundVertexes;
		IndexVector		undergroundIndexes;
		
		ThreadManager&	threadManager;
		
		std::vector<bool> scheduledChanges;
		
		void	regenerateTerrainBuffer();
		void	regenerateUndergroundBuffer();

		vec2i	voxelToChunkPosition(const vec3& position) const noexcept;

		void	moveMap(const vec2i& delta);

		void	enqueueChanges(const vec2i& delta);

		void	enqueueRowChanges(i32 row, std::vector<bool>& scheduled);
		void	enqueueColumnChanges(i32 col, std::vector<bool>& scheduled);
		void	setAdjacentPointers();
};

}	// namespace vox
