#include "VoxelMap.hpp"

#include <iostream>


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

VoxelType	VoxelMap::getVoxelAt(const vec3i& worldVoxel) const noexcept
{
	if (worldVoxel.y <= 0 || worldVoxel.y >= VoxelChunk::chunkDimensions.y)
	{
		return VoxelType::Air;
	}
	vec2i chunk = voxelToChunk(worldVoxel);

	i32 index = (chunk.x - minPositions.x) * squareSize + (chunk.y - minPositions.y);
	if (index < 0 || static_cast<size_t>(index) >= map.size())
	{
		return VoxelType::Air;
	}

	vec3i chunkWorld = map[index].getWorldPos();

	/*	Add one to account for padding	*/
	i32 localX = (worldVoxel.x - chunkWorld.x) + 1;
	i32 localY = (worldVoxel.y - chunkWorld.y) + 1;
	i32 localZ = (worldVoxel.z - chunkWorld.z) + 1;

	if (localX < 1 || localX > VoxelChunk::chunkDimensions.x ||
		localY < 1 || localY > VoxelChunk::chunkDimensions.y ||
		localZ < 1 || localZ > VoxelChunk::chunkDimensions.z)
	{
		return VoxelType::Air;
	}
	return map[index].at(localX, localY, localZ);
}

vec3	VoxelMap::nearestAirVoxel(const vec3i& origin)
{
	const i32 x = origin.x;
	const i32 y = origin.y;
	const i32 z = origin.z;
	const i32 maxRadius = 255;

	for (i32 r = 1; r <= maxRadius; ++r)
	{
		for (i32 dz = -r; dz <= r; ++dz)
		for (i32 dy = -r; dy <= r; ++dy)
		for (i32 dx = -r; dx <= r; ++dx)
		{
			const bool onSurface =
				(dx == -r || dx == r) ||
				(dy == -r || dy == r) ||
				(dz == -r || dz == r);

			if (!onSurface)
				continue;

			vec3i v{ x + dx, y + dy, z + dz };
			if (getVoxelAt(v) == VoxelType::Air)
			{
				return vec3{ v.x + 0.5f, v.y + 0.5f, v.z + 0.5f };
			}
		}
	}
	return vec3{origin.x + 0.5f, origin.y + 0.5f, origin.z + 0.5f};
}

vec3	VoxelMap::detectCollision(const vec3& origin, const vec3& movement)
{
	constexpr float stepSize = 0.01f;
	const vec3	moveTo = origin + movement;
	const vec3	movementStep = movement.normalized() * stepSize;
	vec3	nonBlockedMovement;
	vec3	position = origin;
	const float	steps = movement.length();
	float moved;

	std::cout << "\nmovement length: " << steps << std::endl;
	std::cout << "we are at: " << origin << std::endl;
	std::cout << "move to: " << moveTo << std::endl;

	vec3i currentVoxel = roundyRound(origin);
	if (getVoxelAt(currentVoxel) != VoxelType::Air)
	{
		return nearestAirVoxel(currentVoxel) - origin;
	}
	for (moved = 0.0f; moved < steps; moved += stepSize)
	{
		const vec3 nextPosition = position + movementStep;
		const vec3i roundedNext = roundyRound(nextPosition);
		if (roundedNext != currentVoxel)
		{
			VoxelType voxel = getVoxelAt(roundedNext);
			if (voxel != VoxelType::Air)
			{
				std::cout << "BONK! at: " << nextPosition << " (" << roundedNext << ")" << std::endl;
				std::cout << "voxel type: " << static_cast<i32>(voxel) << std::endl;
				return nearestAirVoxel(currentVoxel) - origin;
			}
			currentVoxel = roundedNext;
		}
		position = nextPosition;
	}
	if (moved >= steps)
	{
		return movement;
	}
	nonBlockedMovement = position - origin;
	std::cout << "attempted movement: " << movement << std::endl;
	std::cout << "non blocked movement: " << nonBlockedMovement << std::endl;
	return nonBlockedMovement;
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
