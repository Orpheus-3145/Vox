#include "VoxelMap.hpp"

#include <iostream>


namespace vox {

/*	From -x (left/west) to +x (right/east) horizontally, y (up/north) to -y (down/south) vertically. */

bool	VoxelMap::update(const vec3& newPosition)
{
	vec2i	moveDirection = voxelToChunkPosition(newPosition) - playerOnChunk;
	Stopwatch timer;

	timer.start();
	if (moveDirection == vec2i::zero())
	{
		return false;
	}
	playerOnChunk = playerOnChunk + moveDirection;
	rawPosition = newPosition;
	if (squareSize < 2)
	{
		return false;
	}
	while (moveDirection.depth > 0)
	{
		north();
		moveDirection.depth--;
	}
	while (moveDirection.width > 0)
	{
		east();
		moveDirection.width--;
	}
	while (moveDirection.depth < 0)
	{
		south();
		moveDirection.depth++;
	}
	while (moveDirection.width < 0)
	{
		west();
		moveDirection.width++;
	}
	timer.stop();
	threadManager.waitIdle();
	std::cout << "regeneration took: " << timer << std::endl;
	// std::cout << "New map limits: " << minPositions << " to " << maxPositions << std::endl;
	assert(minPositions.x + squareSize - 1 == maxPositions.x && "Error: min/max X don't line up");
	assert(minPositions.y + squareSize - 1 == maxPositions.y && "Error: min/max Y don't line up");
	return true;
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
	minPositions.y += 1;
	maxPositions.y += 1;

	std::rotate(map.begin(), map.begin() + squareSize, map.end());
	setAdjacentPointers();

	const i32 bottomRowIndex = squareSize * (squareSize - 1);
	generateRow(bottomRowIndex);
	meshRow(bottomRowIndex);
	meshRow(bottomRowIndex - squareSize);
}

void	VoxelMap::south()
{
	minPositions.y -= 1;
	maxPositions.y -= 1;
	
    std::rotate(map.begin(), map.end() - squareSize, map.end());
	setAdjacentPointers();
	
	generateRow(0);
	meshRow(0);
	meshRow(0 + squareSize);
}

void	VoxelMap::west()
{
	minPositions.x -= 1;
	maxPositions.x -= 1;

	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + (squareSize - 1), begin + squareSize);
	}
	setAdjacentPointers();

	generateColumn(0);
	meshColumn(0);
	meshColumn(1);
}

void	VoxelMap::east()
{
	minPositions.x += 1;
	maxPositions.x += 1;

	for (i32 row = 0; row < squareSize; row++)
	{
		auto begin = map.begin() + row * squareSize;
		std::rotate(begin, begin + 1, begin + squareSize);
	}
	setAdjacentPointers();

	generateColumn(squareSize - 1);
	meshColumn(squareSize - 1);
	meshColumn(squareSize - 2);
}

}	//namespace vox
