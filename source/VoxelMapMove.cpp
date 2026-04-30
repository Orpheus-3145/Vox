#include "VoxelMap.hpp"

#include <iostream>


namespace vox {

/*	From -x (left/west) to +x (right/east) horizontally, y (up/north) to -y (down/south) vertically. */

VoxelType	VoxelMap::getVoxelAt(const vec3& location)
{
	vec2i chunk = voxelToChunkPosition(location);
	ui32 index = chunk.x * squareSize + chunk.y;
	vec3i	voxelLoc = {
		static_cast<i32>(location.x),
		static_cast<i32>(location.y),
		static_cast<i32>(location.z)
	};
	vec3i	chunkLoc = voxelLoc - map[index].getWorldPos();

	return map[index].at(chunkLoc.x, chunkLoc.y, chunkLoc.z);
}

void	VoxelMap::detectCollision(vec3& movement)
{
	const vec3	movementNorm = movement.normalized();
	vec3	position = rawPosition;
	float	steps = movement.length();

	for (ui32 i = 0; i < static_cast<ui32>(steps); i++)
	{
		position += movementNorm;
		if (getVoxelAt(position) != VoxelType::Air)
		{
			movement = position - movementNorm;
			return ;
		}
	}
}

bool	VoxelMap::update(const vec3& newPosition)
{
	Stopwatch	timer;
	vec2i		delta = voxelToChunkPosition(newPosition) - playerOnChunk;

	if (delta == vec2i::zero())
	{
		return false;
	}
	timer.start();
	playerOnChunk = playerOnChunk + delta;
	minPositions = minPositions + delta;
	maxPositions = maxPositions + delta;
	rawPosition = newPosition;
	assert(squareSize >= 2 && "squaresize too small");
	vec2i remaining = delta;

	while (remaining.depth > 0) { north(); remaining.depth--; }
	while (remaining.width > 0) { east();  remaining.width--; }
	while (remaining.depth < 0) { south(); remaining.depth++; }
	while (remaining.width < 0) { west();  remaining.width++; }

	setAdjacentPointers();
	enqueueMeshing(delta);
	updateModel();
	threadManager.waitIdle();

	timer.stop();
	std::cout << "regeneration took: " << timer << std::endl;
	assert(minPositions.x + squareSize - 1 == maxPositions.x && "Error: min/max X don't line up");
	assert(minPositions.y + squareSize - 1 == maxPositions.y && "Error: min/max Y don't line up");
	return true;
}

void	VoxelMap::enqueueRowMeshes(i32 row, std::vector<bool>& scheduled)
{
	i32 index = row * squareSize;

	for (i32 col = 0; col < squareSize; col++)
	{
		scheduled[index] = true;
		index++;
	}
}

void	VoxelMap::enqueueColumnMeshes(i32 col, std::vector<bool>& scheduled)
{
	i32 index = col;

	for (i32 row = 0; row < squareSize; row++)
	{
		scheduled[index] = true;
		index += squareSize;
	}
}

void	VoxelMap::enqueueMeshing(const vec2i& delta)
{
	static std::vector<bool> scheduled(static_cast<size_t>(squareSize * squareSize));

	std::fill(scheduled.begin(), scheduled.end(), false);

	/*	Add all north moves	*/
	for (i32 step = 0; step < delta.depth; step++)
	{
		const i32 bottomRow = squareSize - 1 - step;
		enqueueRowMeshes(bottomRow, scheduled);
		enqueueRowMeshes(bottomRow - 1, scheduled);
	}

	/*	Add all south moves	*/
	for (i32 step = 0; step < -delta.depth; step++)
	{
		const i32 topRow = step;
		enqueueRowMeshes(topRow, scheduled);
		enqueueRowMeshes(topRow + 1, scheduled);
	}

	/*	Add all east moves	*/
	for (i32 step = 0; step < delta.width; step++)
	{
		const i32 rightCol = squareSize - 1 - step;
		enqueueColumnMeshes(rightCol, scheduled);
		enqueueColumnMeshes(rightCol - 1, scheduled);
	}

	/*	Add all west moves	*/
	for (i32 step = 0; step < -delta.width; step++)
	{
		const i32 leftCol = step;
		enqueueColumnMeshes(leftCol, scheduled);
		enqueueColumnMeshes(leftCol + 1, scheduled);
	}

	for (size_t i = 0; i < scheduled.size(); i++)
	{
		if (scheduled[i] == true)
		{
			VoxelChunk* c = &map[i];
			threadManager.enqueue([c] {
				c->generateVertexes();
			});
		}
	}
	threadManager.waitIdle();
}

void	VoxelMap::generateRow(i32 index)
{
	const i32 Ycoord = minPositions.y + index / squareSize;

	for (i32 i = 0; i < squareSize; i++)
	{
		map[index].setLocation({minPositions.x + i, Ycoord});
		map[index].generateMap();
		index++;
	}
}

void	VoxelMap::generateColumn(i32 index)
{
	const i32 Xcoord = minPositions.x + index % squareSize;

	for (i32 i = 0; i < squareSize; i++)
	{
		map[index].setLocation({Xcoord, minPositions.y + i});
		map[index].generateMap();
		index += squareSize;
	}
}

void	VoxelMap::north()
{
	std::rotate(map.begin(), map.begin() + squareSize, map.end());
	generateRow(squareSize * (squareSize - 1));
}

void	VoxelMap::south()
{
	std::rotate(map.begin(), map.end() - squareSize, map.end());
	generateRow(0);
}

void	VoxelMap::west()
{
	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + (squareSize - 1), begin + squareSize);
	}
	generateColumn(0);
}

void	VoxelMap::east()
{
	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + 1, begin + squareSize);
	}
	generateColumn(squareSize - 1);
}

}	//namespace vox
