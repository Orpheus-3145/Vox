#pragma once

#include <mutex>

#include "ThreadManager.hpp"
#include "Vectors.hpp"
#include "VoxelChunk.hpp"
#include "World.hpp"


namespace vox {


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
		vec3	detectCollision(const vec3& origin, const vec3& movement);
		
		static inline	std::mutex	lock;
		
	private:
		std::vector<VoxelChunk>			map;
		std::vector<ve::VertexVector*>	terrainVertexes;
		std::vector<ve::VertexVector*>	undergroundVertexes;
	
		i32 	squareSize;
		vec2i	minPositions;
		vec2i	maxPositions;
		vec2i	playerOnChunk;
		
		ThreadManager&	threadManager;
		
		std::vector<bool> scheduledChanges;
		
		vec2i	voxelToChunkPosition(const vec3& position) const noexcept;

		void	moveMap(const vec2i& delta);

		void	enqueueChanges(const vec2i& delta);

		void	enqueueRowChanges(i32 row, std::vector<bool>& scheduled);
		void	enqueueColumnChanges(i32 col, std::vector<bool>& scheduled);
		void	setAdjacentPointers();

		vec3	nearestAirVoxel(const vec3i& origin);
		bool	testVoxels(const vec3& location);
};

}	// namespace vox
