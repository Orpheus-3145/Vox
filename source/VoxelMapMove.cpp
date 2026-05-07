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

	moveMap(delta);
	setAdjacentPointers();
	enqueueChanges(delta);

	vec2i	pos = minPositions;
	ui32	index = 0;
	for (i32 z = 0; z < squareSize; z++)
	{
		pos.x = minPositions.x;
		for (i32 x = 0; x < squareSize; x++)
		{
			if (scheduledChanges[index] == true)
			{
				VoxelChunk* c = &map[index];
				map[index].setLocation(pos);
				threadManager.enqueue([c] {
					c->generateMap();
				});
			}
			pos.x++;
			index++;
		}
		pos.y++;
	}
	threadManager.waitIdle();
	for (size_t i = 0; i < scheduledChanges.size(); i++)
	{
		if (scheduledChanges[i] == true)
		{
			VoxelChunk* c = &map[i];
			threadManager.enqueue([c] {
				c->generateVertexes();
			});
		}
	}
	threadManager.waitIdle();
	threadManager.enqueue([this] { regenerateTerrainBuffer(); });
	threadManager.enqueue([this] { regenerateUndergroundBuffer(); });
	threadManager.waitIdle();
	timer.stop();
	std::cout << "regeneration took: " << timer << std::endl;
	assert(minPositions.x + squareSize - 1 == maxPositions.x && "Error: min/max X don't line up");
	assert(minPositions.y + squareSize - 1 == maxPositions.y && "Error: min/max Y don't line up");
	return true;
}

void	VoxelMap::enqueueRowChanges(i32 row, std::vector<bool>& scheduled)
{
	i32 index = row * squareSize;

	for (i32 col = 0; col < squareSize; col++)
	{
		scheduled[index] = true;
		index++;
	}
}

void	VoxelMap::enqueueColumnChanges(i32 col, std::vector<bool>& scheduled)
{
	i32 index = col;

	for (i32 row = 0; row < squareSize; row++)
	{
		scheduled[index] = true;
		index += squareSize;
	}
}

void	VoxelMap::enqueueChanges(const vec2i& delta)
{
	const i32 moveEastWest = delta.x;
	const i32 moveNorthSouth = delta.y;

	std::fill(scheduledChanges.begin(), scheduledChanges.end(), false);
	/*	Add all north moves	*/
	for (i32 step = 0; step < moveNorthSouth; step++)
	{
		const i32 bottomRow = squareSize - 1 - step;
		enqueueRowChanges(bottomRow, scheduledChanges);
		enqueueRowChanges(bottomRow - 1, scheduledChanges);
	}
	/*	Add all south moves	*/
	for (i32 step = 0; step < -moveNorthSouth; step++)
	{
		const i32 topRow = step;
		enqueueRowChanges(topRow, scheduledChanges);
		enqueueRowChanges(topRow + 1, scheduledChanges);
	}
	/*	Add all east moves	*/
	for (i32 step = 0; step < moveEastWest; step++)
	{
		const i32 rightCol = squareSize - 1 - step;
		enqueueColumnChanges(rightCol, scheduledChanges);
		enqueueColumnChanges(rightCol - 1, scheduledChanges);
	}
	/*	Add all west moves	*/
	for (i32 step = 0; step < -moveEastWest; step++)
	{
		const i32 leftCol = step;
		enqueueColumnChanges(leftCol, scheduledChanges);
		enqueueColumnChanges(leftCol + 1, scheduledChanges);
	}
}

void	VoxelMap::moveMap(const vec2i& delta)
{
	i32 moveEastWest = delta.x;
	i32 moveNorthSouth = delta.y;

	/*	rotate west	*/
	if (moveEastWest < 0)
	{
		moveEastWest *= -1;
		for (i32 row = 0; row < squareSize; row++)
		{
			auto begin = map.begin() + row * squareSize;
			std::rotate(begin, begin + (squareSize - moveEastWest), begin + squareSize);
		}
	}
	/*	rotate east	*/
	else if (moveEastWest > 0)
	{
		for (i32 row = 0; row < squareSize; row++)
		{
			auto begin = map.begin() + row * squareSize;
			std::rotate(begin, begin + moveEastWest, begin + squareSize);
		}
	}
	/*	rotate south	*/
	if (moveNorthSouth < 0)
	{
		moveNorthSouth *= -1;
		std::rotate(map.begin(), map.end() - squareSize * moveNorthSouth, map.end());
	}
	/*	rotate north	*/
	else if (moveNorthSouth > 0)
	{
		std::rotate(map.begin(), map.begin() + squareSize * moveNorthSouth, map.end());
	}
}

}	//namespace vox
