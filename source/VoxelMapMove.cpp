#include "VoxelMap.hpp"

#include <iostream>


namespace vox {

/*	From -x (left/west) to +x (right/east) horizontally, y (up/north) to -y (down/south) vertically. */

VoxelType	VoxelMap::getVoxelAt(const vec3& location)
{
	vec2i chunk = voxelToChunkPosition(location);

	// std::cout << "location: " << location << std::endl;
	std::cout << "chunk: " << chunk << std::endl;
	// std::cout << "min positions: " << minPositions << std::endl;
	i32 index = (chunk.x - minPositions.x) * squareSize + chunk.y - minPositions.y;

	// std::cout << "index in voxelchunk array: " << index << " (size: " << map.size() << ")" << std::endl;
	vec3i	voxelLoc = {
		static_cast<i32>(location.x),
		static_cast<i32>(location.y),
		static_cast<i32>(location.z)
	};
	if (voxelLoc.y > 255 || voxelLoc.y <= 0)
	{
		return VoxelType::Air;
	}
	std::cout << "voxel location: " << location << std::endl;

	std::cout << "world position of chunk: " << map[index].getWorldPos() << std::endl;
	vec3i	chunkLoc = voxelLoc - map[index].getWorldPos();

	std::cout << "chunk location: " << chunkLoc << std::endl;

	return map[index].at(chunkLoc.x, chunkLoc.y, chunkLoc.z);
}

void	VoxelMap::detectCollision(vec3& movement)
{
	static vec3 previous = vec3::zero();
	const vec3	moveTo = rawPosition + movement;
	const vec3	movementNorm = movement.normalized();
	vec3	position = rawPosition;
	float	steps = movement.length();

	std::cout << "\nwe are at: " << rawPosition << std::endl;
	std::cout << "movement: " << movement << " length: " << steps << std::endl;
	std::cout << "move to: " << moveTo << std::endl;
	std::cout << "single step: " << movementNorm << std::endl;

	for (ui32 i = 0; i < static_cast<ui32>(steps); i++)
	{
		position += movementNorm;
		if (getVoxelAt(position) != VoxelType::Air)
		{
			if (i == 0)
			{
				movement = vec3::zero();
			}
			else
			{
				movement = position - movementNorm;
			}
			break ;
		}
	}
	if (previous != vec3::zero() && vec3{moveTo - previous}.length() > 50.0f)
	{
		std::cerr << "moved from: " << previous << " to " << moveTo << " for a length of " << vec3{moveTo - previous}.length() << std::endl;
		throw std::runtime_error("JUMPED");
	}
	std::cout << "movement: " << movement << std::endl;
	previous = moveTo;
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

	std::cout << "moving by: " << delta << std::endl;

	if (delta.depth > 0)
	{
		north(delta.depth);
	}
	if (delta.width > 0)
	{
		east(delta.width);
	}
	if (delta.depth < 0)
	{
		south(delta.depth);
	}
	if (delta.width < 0)
	{
		west(delta.width);
	}

	setAdjacentPointers();
	// enqueueMeshing(delta);

	timer.stop();
	std::cout << "regeneration took: " << timer << std::endl;
	assert(minPositions.x + squareSize - 1 == maxPositions.x && "Error: min/max X don't line up");
	assert(minPositions.y + squareSize - 1 == maxPositions.y && "Error: min/max Y don't line up");
	return true;
}

// void	VoxelMap::enqueueRowMeshes(i32 row, std::vector<bool>& scheduled)
// {
// 	i32 index = row * squareSize;

// 	for (i32 col = 0; col < squareSize; col++)
// 	{
// 		scheduled[index] = true;
// 		index++;
// 	}
// }

// void	VoxelMap::enqueueColumnMeshes(i32 col, std::vector<bool>& scheduled)
// {
// 	i32 index = col;

// 	for (i32 row = 0; row < squareSize; row++)
// 	{
// 		scheduled[index] = true;
// 		index += squareSize;
// 	}
// }

// void	VoxelMap::enqueueMeshing(const vec2i& delta)
// {
// 	static std::vector<bool> scheduled(static_cast<size_t>(squareSize * squareSize));

// 	std::fill(scheduled.begin(), scheduled.end(), false);

// 	/*	Add all north moves	*/
// 	for (i32 step = 0; step < delta.depth; step++)
// 	{
// 		const i32 bottomRow = squareSize - 1 - step;
// 		enqueueRowMeshes(bottomRow, scheduled);
// 		enqueueRowMeshes(bottomRow - 1, scheduled);
// 	}

// 	/*	Add all south moves	*/
// 	for (i32 step = 0; step < -delta.depth; step++)
// 	{
// 		const i32 topRow = step;
// 		enqueueRowMeshes(topRow, scheduled);
// 		enqueueRowMeshes(topRow + 1, scheduled);
// 	}

// 	/*	Add all east moves	*/
// 	for (i32 step = 0; step < delta.width; step++)
// 	{
// 		const i32 rightCol = squareSize - 1 - step;
// 		enqueueColumnMeshes(rightCol, scheduled);
// 		enqueueColumnMeshes(rightCol - 1, scheduled);
// 	}

// 	/*	Add all west moves	*/
// 	for (i32 step = 0; step < -delta.width; step++)
// 	{
// 		const i32 leftCol = step;
// 		enqueueColumnMeshes(leftCol, scheduled);
// 		enqueueColumnMeshes(leftCol + 1, scheduled);
// 	}

// 	for (size_t i = 0; i < scheduled.size(); i++)
// 	{
// 		if (scheduled[i] == true)
// 		{
// 			VoxelChunk* c = &map[i];
// 			threadManager.enqueue([c] {
// 				c->generateVertexes();
// 			});
// 		}
// 	}
// 	threadManager.waitIdle();
// }

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

void	VoxelMap::meshRow(i32 index)
{
	for (i32 i = 0; i < squareSize; i++)
	{
		map[index].generateVertexes();
		index++;
	}
}

void	VoxelMap::meshColumn(i32 index)
{
	for (i32 i = 0; i < squareSize; i++)
	{
		map[index].generateVertexes();
		index += squareSize;
	}
}

void	VoxelMap::north(i32 moves)
{
	i32 i;
	puts("North");
	std::rotate(map.begin(), map.begin() + squareSize * moves, map.end());
	setAdjacentPointers();
	for (i = 1; i <= moves; i++)
	{
		generateRow(squareSize * (squareSize - i));
	}
	for (i = 1; i <= moves; i++)
	{
		meshRow(squareSize * (squareSize - i));
	}
	meshRow(squareSize * (squareSize - moves - 1));
}

void	VoxelMap::south(i32 moves)
{
	i32 i;
	const i32 steps = moves * -1;

	puts("South");
	std::rotate(map.begin(), map.end() - squareSize * steps, map.end());
	setAdjacentPointers();
	for (i = 0; i < steps; i++)
	{
		generateRow(i * squareSize);
	}
	for (i = 0; i < steps; i++)
	{
		meshRow(i * squareSize);
	}
	meshRow(steps * squareSize);
}

void	VoxelMap::west(i32 moves)
{
	i32 i;
	const int steps = moves * -1;

	puts("West");
	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + (squareSize - steps), begin + squareSize);
	}
	setAdjacentPointers();
	for (i = 0; i < steps; i++)
	{
		generateColumn(i);
	}
	for (i = 0; i < steps; i++)
	{
		meshColumn(i);
	}
	meshColumn(steps);
}

void	VoxelMap::east(i32 moves)
{
	i32 i;
	puts("East");
	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + moves, begin + squareSize);
	}
	setAdjacentPointers();
	for (i = 1; i <= moves; i++)
	{
		generateColumn(squareSize - i);
	}
	for (i = 1; i <= moves; i++)
	{
		meshColumn(squareSize - i);
	}
	meshColumn(squareSize - moves - 1);
}

}	//namespace vox
