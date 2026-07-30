#pragma once

#include <array>
#include <vector>

#include "VulkanModel.hpp"
#include "Vectors.hpp"
#include "NoiseGenerator.hpp"
#include "World.hpp"


namespace vox {

enum class VoxelType : ui8
{
	Air = 0,
	Dirt = 1,
	Stone = 2,
	Water = 3,
	Padding = 255
};

enum VertexFaces : size_t
{
	FRONT = 0,
	BACK = 4,
	LEFT = 8,
	RIGHT = 12,
	TOP = 16,
	BOTTOM = 20
};

enum class Direction : ui8
{
	North,
	East,
	South,
	West
};

class VoxelChunk
{
	public:

		VoxelChunk() = delete;
		VoxelChunk(vec2i loc, VertexVector* terrainVertexes, VertexVector* undergroundVertexes);
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

		const vec3i& getWorldPos() const noexcept { return this->worldPosition; }
		
		void	setAdjacentChunks(VoxelChunk* north, VoxelChunk* east, VoxelChunk* south, VoxelChunk* west) noexcept;
		void	setLocation(vec2i loc);

		i32		index(i32 x, i32 y, i32 z) const noexcept { return ((z * paddedDimensions.x) + x) * paddedDimensions.y + y; }
		VoxelType	at(i32 x, i32 y, i32 z) const noexcept { return map[index(x, y, z)]; }
		VoxelType	at(i32 index) const noexcept { return map[index]; }
		const VoxelType*	dataAt(i32 index) const noexcept { return map.data() + index; }

		bool	testForCollision(const std::vector<vec3i>& locations) const noexcept;

	private:
		void	addVoxelFace(const vec3& voxelLocation, size_t faceIndex, i32 voxelIndex, uint32_t textureIndex);
		void	copyAdjacentData();

		vec2i			location;
		vec3i			worldPosition;
		NoiseGenerator	generator;

		std::vector<VoxelType>		map;
		VertexVector*				terrainVertexes{nullptr};
		VertexVector*				undergroundVertexes{nullptr};
		std::array<VoxelChunk*, 4>	adjacentChunks{};
};

}	// namespace vox
