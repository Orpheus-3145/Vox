#pragma once

#include <array>
#include <vector>

#include "VulkanModel.hpp"
#include "Vectors.hpp"
#include "NoiseGenerator.hpp"
#include "TypeAliases.hpp"


namespace vox {

enum class VoxelType : ui8
{
	Air = 0,
	Dirt = 1,
	Stone = 2,
	Water = 3,
	Padding = 255
};

class VoxelChunk
{
	public:

		VoxelChunk() = delete;
		VoxelChunk(vec2i loc);
		~VoxelChunk() = default;
		VoxelChunk(const VoxelChunk&) = delete;
		VoxelChunk(VoxelChunk&&) noexcept = default;
		VoxelChunk& operator=(VoxelChunk&&) noexcept = default;
		VoxelChunk& operator=(const VoxelChunk&) = delete;

		static vec3i	chunkDimensions;
		static vec3i	paddedDimensions;
		static ui32		paddedSize;
		static ui32		chunkSize;

		void	generateMap();
		void	generateVertexes();

		const VertexVector&	getVertexTerrainData() const noexcept { return terrainVertexes; }
		const VertexVector&	getVertexUndergroundData() const noexcept { return undergroundVertexes; }

		void	setAdjacentChunks(VoxelChunk* north, VoxelChunk* east, VoxelChunk* south, VoxelChunk* west) noexcept;
		void	setLocation(vec2i loc);

		size_t	getVertexTerrainSize() const noexcept { return terrainVertexes.size(); }
		size_t	getVertexUndergroundSize() const noexcept { return undergroundVertexes.size(); }
		i32		index(i32 x, i32 y, i32 z) const noexcept { return ((z * paddedDimensions.x) + x) * paddedDimensions.y + y; }
		VoxelType	at(i32 x, i32 y, i32 z) const noexcept { return map[index(x, y, z)]; }
		VoxelType	at(i32 index) const noexcept { return map[index]; }
		const VoxelType*	dataAt(i32 index) const noexcept { return map.data() + index; }

	private:
		void	addVoxelFace(const vec3& voxelLocation, size_t faceIndex, i32 voxelIndex);
		void	copyAdjacentData();

		vec2i			location;
		vec3i			worldPosition;
		NoiseGenerator	generator;

		std::vector<VoxelType>		map;
		VertexVector				terrainVertexes;
		VertexVector				undergroundVertexes;
		std::array<VoxelChunk*, 4>	adjacentChunks{};
};

}	// namespace vox
