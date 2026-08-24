#include <map>
#include <iostream>

#include "World.hpp"
#include "Config.hpp"
#include "Utils.hpp"


namespace vox {

ve::VertexVector voxelVertexes( vec3 const& relativeOrigin )
{
	ve::VertexVector vertexes{VOXEL_VERTEXES.begin(), VOXEL_VERTEXES.end()};

	for (size_t i = 0U; i < VERTEX_PER_VOXEL; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

ve::VertexVector voxelAtlasVertexes( vec3 const& relativeOrigin )
{
	ve::VertexVector vertexes{VOXEL_VERTEXES_ATLAS.begin(), VOXEL_VERTEXES_ATLAS.end()};

	for (size_t i = 0U; i < VERTEX_PER_VOXEL; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

ve::VertexVector voxelFaceVertexes( vec3 const& relativeOrigin, VoxelFace face )
{
	auto firstFaceVertex = VOXEL_VERTEXES.begin() + static_cast<i8>(face);
	auto lastFaceVertex = firstFaceVertex + VERTEX_PER_FACE;
	ve::VertexVector vertexes(firstFaceVertex, lastFaceVertex);

	for (size_t i = 0U; i < VERTEX_PER_FACE; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

ve::VertexVector voxelFaceAtlasVertexes( vec3 const& relativeOrigin, VoxelFace face )
{
	auto firstFaceVertex = VOXEL_VERTEXES_ATLAS.begin() + static_cast<i8>(face);
	auto lastFaceVertex = firstFaceVertex + VERTEX_PER_FACE;
	ve::VertexVector vertexes(firstFaceVertex, lastFaceVertex);

	for (size_t i = 0U; i < VERTEX_PER_FACE; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

ve::IndexVector voxelIndexes( ui32 start )
{
	ve::IndexVector indexes{VOXEL_INDEXES.begin(), VOXEL_INDEXES.end()};

	for (size_t i = 0U; i < INDEX_PER_VOXEL; i++)
	{
		indexes[i] += start;
	}
	return indexes;
}

ve::IndexVector voxelFaceIndexes( ui32 start )
{
	ve::IndexVector indexes{FACE_INDEXES.begin(), FACE_INDEXES.end()};

	for (size_t i = 0U; i < INDEX_PER_FACE; i++)
	{
		indexes[i] += start;
	}
	return indexes;
}


World::World( vec2i const& indexWorld, vec3ui const& worldSize, WorldNavigator& navigator, NoiseGenerator const& generator ) :
	indexWorld(indexWorld),
	worldSize(worldSize),
	navigator(navigator),
	generator(generator),
	terrainObject(),
	caveObject()
{
	this->map.resize(this->worldSize.width * this->worldSize.height * this->worldSize.depth);
	this->setLastAccess();
}

void World::createMap( void )
{
	std::fill(this->map.begin(), this->map.end(), VoxelType::Air);
	for (ui32 z = 0U; z < this->worldSize.depth; z++)
	{
		for (ui32 x = 0U; x < this->worldSize.width; x++)
		{
			vec3	globalPos = this->getRealWorldPos(x, 0U, z) / VOXEL_SIZE;
			float	perlinValue = this->generator.octavePerlin2D(globalPos.x, globalPos.z);
			ui32	heightValue = static_cast<ui32>(perlinValue * this->worldSize.height);

			for (ui32 y = 0U; y < heightValue; y++)
			{
				perlinValue = this->generator.octavePerlin3D(globalPos.x, static_cast<float>(y), globalPos.z);
				float t = static_cast<float>(y) / static_cast<float>(this->worldSize.height);
				float factor = t * t * (3 - 2 * t);
				float treshold = 0.575f + 0.2f * factor;

				if (perlinValue <= treshold)	// if perlinValue>treshold leave it empty => create the cave
				{
					if (y > heightValue - 4)
					{
						this->map[this->pos3DtoIndex(x, y, z)] = VoxelType::Dirt;
					}
					else
					{
						this->map[this->pos3DtoIndex(x, y, z)] = VoxelType::Stone;
					}
				}
			}
		}
	}
	std::cout << "new world in " << this->indexWorld << std::endl;
}

ve::VertexVector World::createTerrainVertexes( bool applyFaceCulling ) const
{
	ve::VertexVector vertexes, buffer;

	for (ui32 x = 0U; x < this->worldSize.width; x++)
	{
		for (ui32 z = 0U; z < this->worldSize.depth; z++)
		{
			for (i32 y = this->worldSize.height - 1U; y >= 0; y--)
			{
				VoxelType voxel = this->getVoxelType(x, static_cast<ui32>(y), z);
				if (voxel == VoxelType::Air) continue;
				else if (voxel == VoxelType::Stone) break;

				vec3 globalPos = this->getRealWorldPos(x, static_cast<ui32>(y), z);
				if (applyFaceCulling)
				{
					std::map<VoxelFace,vec3> surroundings{
						std::pair<VoxelFace,vec3>(VoxelFace::LEFT, vec3{globalPos.x - VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::RIGHT, vec3{globalPos.x + VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::BACK, vec3{globalPos.x, globalPos.y, globalPos.z + VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::FRONT, vec3{globalPos.x, globalPos.y, globalPos.z - VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::BOTTOM, vec3{globalPos.x, globalPos.y - VOXEL_SIZE, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::TOP, vec3{globalPos.x, globalPos.y + VOXEL_SIZE, globalPos.z})
					};

					for (auto const& [faceDirection, position] : surroundings)
					{
						if (this->navigator.getVoxelType(position) != VoxelType::Air) continue;

						buffer = voxelFaceAtlasVertexes(globalPos, faceDirection);
						vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
					}
				}
				else
				{
					buffer = voxelAtlasVertexes(globalPos);
					vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
				}
			}
		}
	}

	std::cout << "new terr vertexes in " << this->indexWorld << std::endl;
	return vertexes;
}

ve::VertexVector World::createCaveVertexes( bool applyFaceCulling ) const
{
	ve::VertexVector vertexes, buffer;

	for (ui32 z = 0U; z < this->worldSize.depth; z++)
	{
		for (ui32 x = 0U; x < this->worldSize.width; x++)
		{
			for (ui32 y = 0U; y < this->worldSize.height; y++)
			{
				VoxelType voxel = this->getVoxelType(x, y, z);
				if (voxel == VoxelType::Air) continue;
				else if (voxel == VoxelType::Dirt) break;

				vec3 globalPos = this->getRealWorldPos(x, static_cast<ui32>(y), z);
				if (applyFaceCulling)
				{
					std::map<VoxelFace,vec3> surroundings{
						std::pair<VoxelFace,vec3>(VoxelFace::LEFT, vec3{globalPos.x - VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::RIGHT, vec3{globalPos.x + VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::BACK, vec3{globalPos.x, globalPos.y, globalPos.z + VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::FRONT, vec3{globalPos.x, globalPos.y, globalPos.z - VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::BOTTOM, vec3{globalPos.x, globalPos.y - VOXEL_SIZE, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::TOP, vec3{globalPos.x, globalPos.y + VOXEL_SIZE, globalPos.z})
					};

					for (auto const& [faceDirection, position] : surroundings)
					{
						if (this->navigator.getVoxelType(position) != VoxelType::Air) continue;

						buffer = voxelFaceVertexes(globalPos, faceDirection);
						vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
					}
				}
				else
				{
					buffer = voxelVertexes(globalPos);
					vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
				}
			}
		}
	}

	std::cout << "new cave vertexes in " << this->indexWorld << std::endl;
	return vertexes;
}

void World::createTerrainVertexesMT( bool applyFaceCulling, ve::VulkanDevice& vulkanDevice )
{
	ve::VertexVector vertexes, buffer;

	for (ui32 x = 0U; x < this->worldSize.width; x++)
	{
		for (ui32 z = 0U; z < this->worldSize.depth; z++)
		{
			for (i32 y = this->worldSize.height - 1U; y >= 0; y--)
			{
				VoxelType voxel = this->getVoxelType(x, static_cast<ui32>(y), z);
				if (voxel == VoxelType::Air) continue;
				else if (voxel == VoxelType::Stone) break;

				vec3 globalPos = this->getRealWorldPos(x, static_cast<ui32>(y), z);
				if (applyFaceCulling)
				{
					std::map<VoxelFace,vec3> surroundings{
						std::pair<VoxelFace,vec3>(VoxelFace::LEFT, vec3{globalPos.x - VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::RIGHT, vec3{globalPos.x + VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::BACK, vec3{globalPos.x, globalPos.y, globalPos.z + VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::FRONT, vec3{globalPos.x, globalPos.y, globalPos.z - VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::BOTTOM, vec3{globalPos.x, globalPos.y - VOXEL_SIZE, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::TOP, vec3{globalPos.x, globalPos.y + VOXEL_SIZE, globalPos.z})
					};

					for (auto const& [faceDirection, position] : surroundings)
					{
						if (this->navigator.getVoxelType(position) != VoxelType::Air) continue;

						buffer = voxelFaceAtlasVertexes(globalPos, faceDirection);
						vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
					}
				}
				else
				{
					buffer = voxelAtlasVertexes(globalPos);
					vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
				}
			}
		}
	}

	ui32 nInstances = vertexes.size() / ((applyFaceCulling) ? VERTEX_PER_FACE : VERTEX_PER_VOXEL);
	ve::IndexVector instanceIndices = (applyFaceCulling) ? voxelFaceIndexes() : voxelIndexes();
	this->terrainObject.setModel(
		std::make_shared<ve::VulkanModel>(
			vulkanDevice,
			vertexes,
			instanceIndices,
			nInstances,
			0U
		)
	);
	size_t expectedMemoryWorld = vertexes.size() * sizeof(ve::Vertex);
	size_t uploadedMemoryWorld = this->terrainObject.getModel()->getBufferSize();
	assert(expectedMemoryWorld == uploadedMemoryWorld and "delta between expected and uploaded memory");
	this->VRAMsize += uploadedMemoryWorld + nInstances * instanceIndices.size() * sizeof(ui32);
}

void World::createCaveVertexesMT( bool applyFaceCulling, ve::VulkanDevice& vulkanDevice )
{
	ve::VertexVector vertexes, buffer;

	for (ui32 z = 0U; z < this->worldSize.depth; z++)
	{
		for (ui32 x = 0U; x < this->worldSize.width; x++)
		{
			for (ui32 y = 0U; y < this->worldSize.height; y++)
			{
				VoxelType voxel = this->getVoxelType(x, y, z);
				if (voxel == VoxelType::Air) continue;
				else if (voxel == VoxelType::Dirt) break;

				vec3 globalPos = this->getRealWorldPos(x, static_cast<ui32>(y), z);
				if (applyFaceCulling)
				{
					std::map<VoxelFace,vec3> surroundings{
						std::pair<VoxelFace,vec3>(VoxelFace::LEFT, vec3{globalPos.x - VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::RIGHT, vec3{globalPos.x + VOXEL_SIZE, globalPos.y, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::BACK, vec3{globalPos.x, globalPos.y, globalPos.z + VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::FRONT, vec3{globalPos.x, globalPos.y, globalPos.z - VOXEL_SIZE}),
						std::pair<VoxelFace,vec3>(VoxelFace::BOTTOM, vec3{globalPos.x, globalPos.y - VOXEL_SIZE, globalPos.z}),
						std::pair<VoxelFace,vec3>(VoxelFace::TOP, vec3{globalPos.x, globalPos.y + VOXEL_SIZE, globalPos.z})
					};

					for (auto const& [faceDirection, position] : surroundings)
					{
						if (this->navigator.getVoxelType(position) != VoxelType::Air) continue;

						buffer = voxelFaceVertexes(globalPos, faceDirection);
						vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
					}
				}
				else
				{
					buffer = voxelVertexes(globalPos);
					vertexes.insert(vertexes.end(), buffer.begin(), buffer.end());
				}
			}
		}
	}
	ui32 nInstances = vertexes.size() / ((applyFaceCulling) ? VERTEX_PER_FACE : VERTEX_PER_VOXEL);
	ve::IndexVector instanceIndices = (applyFaceCulling) ? voxelFaceIndexes() : voxelIndexes();
	this->caveObject.setModel(
		std::make_shared<ve::VulkanModel>(
			vulkanDevice,
			vertexes,
			instanceIndices,
			nInstances,
			0U
		)
	);
	size_t expectedMemoryWorld = vertexes.size() * sizeof(ve::Vertex);
	size_t uploadedMemoryWorld = this->terrainObject.getModel()->getBufferSize();
	assert(expectedMemoryWorld == uploadedMemoryWorld and "delta between expected and uploaded memory");
	this->VRAMsize += uploadedMemoryWorld + nInstances * instanceIndices.size() * sizeof(ui32);
}

VoxelType World::getVoxelType( vec3ui const& index ) const
{
	return this->map.at(this->pos3DtoIndex(index));
}

VoxelType World::getVoxelType( ui32 x, ui32 y, ui32 z ) const
{
	return this->map.at(this->pos3DtoIndex(x, y, z));
}

vec3 World::getRealWorldPos( ui32 x, ui32 y, ui32 z ) const noexcept
{
	return vec3{
		x + this->indexWorld.width * static_cast<float>(this->worldSize.width),
		static_cast<float>(y),
		z + this->indexWorld.depth * static_cast<float>(this->worldSize.depth)
	} * VOXEL_SIZE;
}

vec3 World::getRealWorldPos( vec3ui const& worldPos ) const noexcept
{
	return this->getRealWorldPos(worldPos.x,worldPos.y,worldPos.z);
}

ui32 World::pos3DtoIndex( ui32 x, ui32 y, ui32 z ) const noexcept
{
	assert((x < this->worldSize.width) and (y < this->worldSize.height) and (z < this->worldSize.depth) and "3D index out of bounds");

	return (z * this->worldSize.width + x) * this->worldSize.height + y;
}

ui32 World::pos3DtoIndex( vec3ui const& pos ) const noexcept
{
	assert((pos.x < this->worldSize.width) and (pos.y < this->worldSize.height) and (pos.z < this->worldSize.depth) and "3D index out of bounds");

	return this->pos3DtoIndex(pos.x, pos.y, pos.z);
}

vec3ui World::indexToPos3D( ui32 index ) const noexcept
{
	assert(index < (this->worldSize.width * this->worldSize.height * this->worldSize.depth) and "1D index out of bounds");

	vec3ui pos3D;
	pos3D.z = index / (this->worldSize.width * this->worldSize.height);

	index -= pos3D.z * this->worldSize.width * this->worldSize.height;
	pos3D.x = index / this->worldSize.height;

	index -= pos3D.x * this->worldSize.height;
	pos3D.y = index;

	return pos3D;
}


void WorldNavigator::spawnCloseByWorlds( vec3 const& start )
{
	this->currentWorldPos = this->getIndexWorld(start);

	// add a world, if not existent already, in each of these 9 quadrants
	//  __ __ __
	// |NW|N |NE|
	// |__|__|__|
	// | W| M| E|
	// |__|__|__|
	// |SW|S |SE|
	// |__|__|__|
	//
	std::map<vec2i,bool> surroundings{
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth - 1}, false},	// SW
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth}, false},		// W
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth + 1}, false},	// NW
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth - 1}, false},	// SE
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth}, false},		// E
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth + 1}, false},	// NE
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width, this->currentWorldPos.depth - 1}, false},		// S
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width, this->currentWorldPos.depth + 1}, false},		// N
		std::pair<vec2i,bool>{vec2i{this->currentWorldPos.width, this->currentWorldPos.depth}, false}			// M
	};

	for (auto& [worldPos, updated] : surroundings)
	{
		if (this->doesWorldExist(worldPos)) continue;

		this->addeNewWorld(worldPos);
		updated = true;
	}
	this->worlds.at(this->currentWorldPos).setLastAccess();

	for (auto& [worldPos, updated] : surroundings)
	{
		if (updated == false) continue;

		this->generateModelWorld(worldPos);
	}
}

void WorldNavigator::spawnCloseByWorldsMT( vec3 const& start )
{
	if (this->borderCrossed(start) == false) return;
	this->currentWorldPos = this->getIndexWorld(start);

	// add a world, if not existent already, in each of these 9 quadrants
	//  __ __ __
	// |NW|N |NE|
	// |__|__|__|
	// | W| M| E|
	// |__|__|__|
	// |SW|S |SE|
	// |__|__|__|
	//
	std::vector<vec2i> surroundings{
		vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth - 1},	// SW
		vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth},		// W
		vec2i{this->currentWorldPos.width - 1, this->currentWorldPos.depth + 1},	// NW
		vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth - 1},	// SE
		vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth},		// E
		vec2i{this->currentWorldPos.width + 1, this->currentWorldPos.depth + 1},	// NE
		vec2i{this->currentWorldPos.width, this->currentWorldPos.depth - 1},		// S
		vec2i{this->currentWorldPos.width, this->currentWorldPos.depth + 1},		// N
		vec2i{this->currentWorldPos.width, this->currentWorldPos.depth}			// M
	};
	std::cout<< "start world generation" << std::endl;

	std::vector<vec2i> newWorlds;
	for (vec2i const& worldPos : surroundings)
	{
		if (this->doesWorldExist(worldPos)) continue;

		newWorlds.push_back(worldPos);
		this->worlds.try_emplace(worldPos, worldPos, this->worldSize, *this, this->generator);
		this->orchestrator.enqueue([this, worldPos] {
			this->worlds.at(worldPos).createMap();
		});
	}
	this->worlds.at(this->currentWorldPos).setLastAccess();

	this->orchestrator.waitIdle();
	std::cout<< "end world generation" << std::endl;
	std::cout<< "start vertex generation" << std::endl;

	for (size_t i = 0; i < newWorlds.size(); i++)
	{
		vec2i pos = newWorlds[i];
		this->orchestrator.enqueue([this, pos] {
			this->worlds.at(pos).createTerrainVertexesMT(this->applyFaceCulling, this->vulkanDevice);
			this->worlds.at(pos).createCaveVertexesMT(this->applyFaceCulling, this->vulkanDevice);
			
			while ((this->currentVRAM + this->worlds.at(pos).getVRAMsize()) > this->maxVRAM)
			{
				vec2i worldToDrop = this->findFurthestWorld();
				assert(worldToDrop != pos and "anomaly in the world destruction");
				this->dropWorld(worldToDrop);
			}
			this->currentVRAM += this->worlds.at(pos).getVRAMsize();
		});
	}
	this->orchestrator.waitIdle();

	this->worldReady = true;
}

VoxelType WorldNavigator::getVoxelType( vec3 const& globalPos ) const noexcept
{
	vec2i	worldIndex = this->getIndexWorld(globalPos);
	if ((this->doesWorldExist(worldIndex) == false) or 
		(globalPos.y < 0.0f) or 
		((globalPos.y / VOXEL_SIZE) >= this->worldSize.height))
	{
		return VoxelType::Air;
	}

	vec3ui	worldPos{
		positiveModulo(std::floor(globalPos.x / VOXEL_SIZE), this->worldSize.width),
		positiveModulo(std::floor(globalPos.y / VOXEL_SIZE), this->worldSize.height),
		positiveModulo(std::floor(globalPos.z / VOXEL_SIZE), this->worldSize.depth)
	};
	return this->worlds.at(worldIndex).getVoxelType(worldPos);
}

vec3 WorldNavigator::checkClipping( vec3 const& startPos, vec3 const& direction ) const noexcept
{
	float const lenPath = direction.length();
	vec3 const	movementStep = direction.normalized() * WorldNavigator::STEP_SIZE;
	vec3		position = startPos;

	for (float moved = 0.0f; moved < lenPath; moved += WorldNavigator::STEP_SIZE)
	{
		bool isBlocked = true;
		if (this->checkCollisionRadius(vec3{position.x + movementStep.x, position.y, position.z}))
		{
			position.x += movementStep.x;
			isBlocked = false;
		}
		if (this->checkCollisionRadius(vec3{position.x, position.y + movementStep.y, position.z}))
		{
			position.y += movementStep.y;
			isBlocked = false;
		}
		if (this->checkCollisionRadius(vec3{position.x, position.y, position.z + movementStep.z}))
		{
			position.z += movementStep.z;
			isBlocked = false;
		}

		if (isBlocked) break;
	}
	return position - startPos;
}

void WorldNavigator::drawTerrain( VkCommandBuffer commandBuffer, std::optional<FrustumBox> const& frustum ) const noexcept
{
	for (auto& [index, chunk] : this->terrain)
	{
		if ((this->applyFrustumCulling == false) or this->isWorldVisible(index, frustum.value()))
		{
			chunk.bindBuffer(commandBuffer);
			chunk.draw(commandBuffer);
		}
	}
}

void WorldNavigator::drawCaves( VkCommandBuffer commandBuffer, std::optional<FrustumBox> const& frustum ) const noexcept
{
	for (auto& [index, chunk] : this->cave)
	{
		if ((this->applyFrustumCulling == false) or this->isWorldVisible(index, frustum.value()))
		{
			chunk.bindBuffer(commandBuffer);
			chunk.draw(commandBuffer);
		}
	}
}

void WorldNavigator::addeNewWorld( vec2i const& worldIndex )
{
	assert(this->doesWorldExist(worldIndex) == false and "world already exists");

	std::cout<< "adding world in: (thread) " << worldIndex << std::endl;
	this->worlds.try_emplace(worldIndex, worldIndex, this->worldSize, *this, this->generator);
	this->worlds.at(worldIndex).createMap();
	this->terrain.try_emplace(worldIndex);
	this->cave.try_emplace(worldIndex);
}

void WorldNavigator::generateModelWorld( vec2i const& worldIndex )
{
	assert(this->doesWorldExist(worldIndex) and "world doesn't exist");

	std::cout<< "creating vertexes in: (thread) " << worldIndex << std::endl;
	ve::VertexVector terrainVertexes = this->worlds.at(worldIndex).createTerrainVertexes(this->applyFaceCulling);
	ve::VertexVector caveVertexes = this->worlds.at(worldIndex).createCaveVertexes(this->applyFaceCulling);

	size_t memoryWorld = (terrainVertexes.size() + caveVertexes.size()) * sizeof(ve::Vertex);							// memory for vertexes
	memoryWorld += (terrainVertexes.size() + caveVertexes.size()) / VERTEX_PER_FACE * INDEX_PER_FACE * sizeof(ui32);	// memory for indices
	while ((this->getMemoryUsed() + memoryWorld) > this->maxVRAM)
	{
		this->dropWorld(this->findFurthestWorld());
	}

	this->terrain.at(worldIndex).setModel(
		std::make_shared<ve::VulkanModel>(
			this->vulkanDevice,
			terrainVertexes,
			voxelFaceIndexes(),
			static_cast<ui32>(terrainVertexes.size() / VERTEX_PER_FACE),
			0U
		)
	);
	this->cave.at(worldIndex).setModel(
		std::make_shared<ve::VulkanModel>(
			this->vulkanDevice,
			caveVertexes,
			voxelFaceIndexes(),
			static_cast<ui32>(caveVertexes.size() / VERTEX_PER_FACE),
			0U
		)
	);

	assert(memoryWorld == (this->terrain.at(worldIndex).getModel()->getBufferSize() + this->cave.at(worldIndex).getModel()->getBufferSize()) and "delta between expected and uploaded memory");
	this->currentVRAM += this->terrain.at(worldIndex).getModel()->getBufferSize();
	this->currentVRAM += this->cave.at(worldIndex).getModel()->getBufferSize();
}

// void WorldNavigator::generateModelWorld( vec2i const& worldIndex, ve::VertexVector const& terrainVertexes, ve::VertexVector const& caveVertexes )
// {
// 	assert(this->doesWorldExist(worldIndex) and "world doesn't exist");

// 	size_t memoryWorld = (terrainVertexes.size() + caveVertexes.size()) * sizeof(ve::Vertex);							// memory for vertexes
// 	memoryWorld += (terrainVertexes.size() + caveVertexes.size()) / VERTEX_PER_FACE * INDEX_PER_FACE * sizeof(ui32);	// memory for indices
// 	while ((this->getMemoryUsed() + memoryWorld) > this->maxVRAM)
// 	{
// 		this->dropWorld(this->findFurthestWorld());
// 	}

// 	this->terrain[worldIndex].setModel(
// 		std::make_shared<ve::VulkanModel>(
// 			this->vulkanDevice,
// 			terrainVertexes,
// 			voxelFaceIndexes(),
// 			static_cast<ui32>(terrainVertexes.size() / VERTEX_PER_FACE),
// 			0U
// 		)
// 	);
// 	this->cave[worldIndex].setModel(
// 		std::make_shared<ve::VulkanModel>(
// 			this->vulkanDevice,
// 			caveVertexes,
// 			voxelFaceIndexes(),
// 			static_cast<ui32>(caveVertexes.size() / VERTEX_PER_FACE),
// 			0U
// 		)
// 	);

// 	assert(memoryWorld == (this->terrain.at(worldIndex).getModel()->getBufferSize() + this->cave.at(worldIndex).getModel()->getBufferSize()) and "delta between expected and uploaded memory");
// 	this->currentVRAM += this->terrain.at(worldIndex).getModel()->getBufferSize();
// 	this->currentVRAM += this->cave.at(worldIndex).getModel()->getBufferSize();
// }

void WorldNavigator::dropWorld( vec2i const& worldToDropIndex )
{
	if (this->doesWorldExist(worldToDropIndex) == false) return;

	this->currentVRAM -= this->terrain.at(worldToDropIndex).getModel()->getBufferSize();
	this->currentVRAM -= this->cave.at(worldToDropIndex).getModel()->getBufferSize();

	this->worlds.erase(worldToDropIndex);
	this->terrain.erase(worldToDropIndex);
	this->cave.erase(worldToDropIndex);
}

bool WorldNavigator::checkCollisionRadius( vec3 const& position ) const noexcept
{
	i32 const minX = static_cast<i32>(std::floor(position.x - WorldNavigator::RADIUS));
	i32 const maxX = static_cast<i32>(std::ceil(position.x + WorldNavigator::RADIUS));
	i32 const minY = static_cast<i32>(std::floor(position.y - WorldNavigator::RADIUS));
	i32 const maxY = static_cast<i32>(std::ceil(position.y + WorldNavigator::RADIUS));
	i32 const minZ = static_cast<i32>(std::floor(position.z - WorldNavigator::RADIUS));
	i32 const maxZ = static_cast<i32>(std::ceil(position.z + WorldNavigator::RADIUS));

	for (i32 x = minX; x < maxX; ++x)
	{
		for (i32 y = minY; y < maxY; ++y)
		{
			for (i32 z = minZ; z < maxZ; ++z)
			{
				const vec3 voxelCenter{
					static_cast<float>(x) + VOXEL_SIZE / 2.0f,
					static_cast<float>(y) + VOXEL_SIZE / 2.0f,
					static_cast<float>(z) + VOXEL_SIZE / 2.0f
				};
				if (this->getVoxelType(voxelCenter) != VoxelType::Air) return false;
			}
		}
	}
	return true;
}

vec2i WorldNavigator::findFurthestWorld( void ) const noexcept
{
	vec2i furthestWorld = this->currentWorldPos;
	float furthestDist = 0.0f;
	for (auto const& [pos, world] : this->worlds)
	{
		float deltaSpace = vec2i::distance(pos, this->currentWorldPos);
		float deltaTime = world.getLastAccess();

		float distance = WorldNavigator::ALPHA * deltaSpace + WorldNavigator::BETA * deltaTime;
		if (distance > furthestDist)
		{
			furthestWorld = pos;
			furthestDist = distance;
		}
	}
	return furthestWorld;
}

vec2i WorldNavigator::getIndexWorld( vec3 const& globalPos ) const noexcept
{
	return vec2i{
		static_cast<i32>(std::floor(globalPos.x / (this->worldSize.width * VOXEL_SIZE))),
		static_cast<i32>(std::floor(globalPos.z / (this->worldSize.depth * VOXEL_SIZE)))
	};
}

static bool isAABBOutsidePlane( Plane const& plane, vec3 const& min, vec3 const& max )
{
	vec3 positiveVertex = min;
	if (plane.normal.x >= 0) positiveVertex.x = max.x;
	if (plane.normal.y >= 0) positiveVertex.y = max.y;
	if (plane.normal.z >= 0) positiveVertex.z = max.z;

	return (vec3::dot(plane.normal, positiveVertex) + plane.distance) < 0;
}

bool WorldNavigator::isWorldVisible( vec2i const& worldIndex, FrustumBox const& frustum ) const
{
	assert(this->doesWorldExist(worldIndex) and "world doesn't exist");

	vec3 minWorld = this->worlds.at(worldIndex).getRealWorldPos(0U, 0U, 0U);
	vec3 maxWorld = this->worlds.at(worldIndex).getRealWorldPos(this->worldSize);

	for (Plane const& plance : frustum)
	{
		if (isAABBOutsidePlane(plance, minWorld, maxWorld)) return false;
	}

	return true;
}

}	// namespace vox
