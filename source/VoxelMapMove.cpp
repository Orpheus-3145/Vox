#include <iostream>

#include "VoxelMap.hpp"


namespace vox {

/*	From -x (left/west) to +x (right/east) horizontally, y (up/north) to -y (down/south) vertically. */

static i32	floorDivision(i32 a, i32 b)
{
	i32 q = a / b;
	i32 r = a % b;
	if (r != 0 && a < 0)
	{
		q -= 1;
	}
	return q;
}

static vec2i	voxelToChunk(const vec3i& voxel)
{
	return vec2i {
		floorDivision(voxel.x, VoxelChunk::chunkDimensions.x),
		floorDivision(voxel.z, VoxelChunk::chunkDimensions.z)
	};
}

static vec3i roundyRound(const vec3& voxel)
{
	return
	{
		static_cast<i32>(std::floor(voxel.x)),
		static_cast<i32>(std::floor(voxel.y)),
		static_cast<i32>(std::floor(voxel.z))
	};
}

vec3	VoxelMap::nearestAirVoxel(const vec3i& origin)
{
	return vec3{origin.x, 255, origin.z};
}

static void	insertLocations(const vec3& position, std::vector<vec3i>& locations)
{
	static constexpr float playerRadius = 0.3f * VOXEL_SIZE;
	static const std::array<vec3, 8>	directions {
		vec3{playerRadius, playerRadius, playerRadius},	// right-up-forward
		vec3{playerRadius, playerRadius, -playerRadius},	// right-up-back
		vec3{playerRadius, -playerRadius, playerRadius},	// right-down-forward
		vec3{playerRadius, -playerRadius, -playerRadius},	// right-down-back
		vec3{-playerRadius, playerRadius, playerRadius},	// left-up-forward
		vec3{-playerRadius, playerRadius, -playerRadius},	// left-up-back
		vec3{-playerRadius, -playerRadius, playerRadius},	// left-down-forward
		vec3{-playerRadius, -playerRadius, -playerRadius},	// left-down-backward
	};

	for (size_t i = 0; i < locations.size(); i++)
	{
		locations[i] = roundyRound(position + directions[i]);
	}
}

bool	VoxelMap::testVoxels(const vec3& location)
{
	static std::vector<vec3i> voxelsToTest(8);

	if (location.y <= 0 || location.y >= VoxelChunk::chunkDimensions.y)
	{
		return false;
	}
	vec2i	chunk = voxelToChunk(roundyRound(location));
	i32		index = (chunk.x - minPositions.x) * squareSize + (chunk.y - minPositions.y);

	if (index < 0 || static_cast<size_t>(index) >= map.size())
	{
		return false;
	}
	vec3	locationOnChunk = vec3{
		std::fmod(location.x, static_cast<float>(VoxelChunk::chunkDimensions.x)),
		location.y,
		std::fmod(location.z, static_cast<float>(VoxelChunk::chunkDimensions.z))
	};
	if (location.x < 0.0f) { locationOnChunk.x += static_cast<float>(VoxelChunk::chunkDimensions.x); }
	if (location.z < 0.0f) { locationOnChunk.z += static_cast<float>(VoxelChunk::chunkDimensions.z); }

	insertLocations(locationOnChunk, voxelsToTest);
	VoxelMap::lock.lock();
	const bool result = map[index].testForCollision(voxelsToTest);
	VoxelMap::lock.unlock();

	return result;
}

vec3	VoxelMap::detectCollision(const vec3& origin, const vec3& movement)
{
	constexpr float stepSize = 0.01f;

	const vec3	movementStep = movement.normalized() * stepSize;
	const float	steps = movement.length();
	vec3	position = origin;
	float	moved;

	const bool collided = testVoxels(origin);
	if (collided == true)
	{
		return nearestAirVoxel(roundyRound(origin)) - origin;
	}
	for (moved = 0.0f; moved < steps; moved += stepSize)
	{
		const vec3 nextPosition = position + movementStep;
		const bool isColliding = testVoxels(nextPosition);
		if (isColliding == true)
		{
			position.x = std::floor(position.x) + 0.5f;
			position.y = std::floor(position.y) + 0.5f;
			position.z = std::floor(position.z) + 0.5f;
			break;
		}
		position = nextPosition;
	}
	return position - origin;
}

bool	VoxelMap::update(const vec3& newPosition)
{
	vec2i		delta = voxelToChunkPosition(newPosition) - playerOnChunk;

	if (delta == vec2i::zero())
	{
		return false;
	}
	playerOnChunk = playerOnChunk + delta;
	minPositions = minPositions + delta;
	maxPositions = maxPositions + delta;
	assert(squareSize >= 2 && "squaresize too small");
	VoxelMap::lock.lock();
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
	VoxelMap::lock.unlock();
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
	threadManager.waitIdle();
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
