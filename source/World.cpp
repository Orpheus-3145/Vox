#include <map>

#include "World.hpp"
#include "Config.hpp"
#include "Utils.hpp"


namespace vox {

VertexVector voxelVertexes( vec3 const& relativeOrigin )
{
	VertexVector vertexes{VOXEL_VERTEXES.begin(), VOXEL_VERTEXES.end()};

	for (ui32 i = 0U; i < ve::VERTEX_PER_VOXEL; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

VertexVector voxelAtlasVertexes( vec3 const& relativeOrigin )
{
	VertexVector vertexes{VOXEL_VERTEXES_ATLAS.begin(), VOXEL_VERTEXES_ATLAS.end()};

	for (ui32 i = 0U; i < ve::VERTEX_PER_VOXEL; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

VertexVector voxelFaceVertexes( vec3 const& relativeOrigin, VoxelFace face )
{
	auto firstFaceVertex = VOXEL_VERTEXES.begin() + static_cast<i8>(face);
	auto lastFaceVertex = firstFaceVertex + ve::VERTEX_PER_FACE;
	VertexVector vertexes(firstFaceVertex, lastFaceVertex);

	for (ui32 i = 0U; i < ve::VERTEX_PER_FACE; i++)
	{
		vertexes[i].pos += relativeOrigin;
		vertexes[i].textureIndex = 1U;
	}
	return vertexes;
}

VertexVector voxelFaceAtlasVertexes( vec3 const& relativeOrigin, VoxelFace face )
{
	auto firstFaceVertex = VOXEL_VERTEXES_ATLAS.begin() + static_cast<i8>(face);
	auto lastFaceVertex = firstFaceVertex + ve::VERTEX_PER_FACE;
	VertexVector vertexes(firstFaceVertex, lastFaceVertex);

	for (ui32 i = 0U; i < ve::VERTEX_PER_FACE; i++)
	{
		vertexes[i].pos += relativeOrigin;
	}
	return vertexes;
}

IndexVector voxelIndexes( ui32 start )
{
	IndexVector indexes{ve::VOXEL_INDEXES.begin(), ve::VOXEL_INDEXES.end()};

	for (ui32 i = 0U; i < ve::INDEX_PER_VOXEL; i++)
	{
		indexes[i] += start;
	}
	return indexes;
}

IndexVector voxelFaceIndexes( ui32 start )
{
	IndexVector indexes{ve::FACE_INDEXES.begin(), ve::FACE_INDEXES.end()};

	for (ui32 i = 0U; i < ve::INDEX_PER_FACE; i++)
	{
		indexes[i] += start;
	}
	return indexes;
}


World::World( vec2i const& indexWorld, vec3ui const& worldSize, WorldNavigator& navigator, ui32 seed ) :
	indexWorld(indexWorld),			// NB fetch values from config or pass them through constructor?
	worldSize(worldSize),
	navigator(navigator),
	generator(seed)
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
			vec3	globalPos = this->getRealWorldPos(x, 0U, z);
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
						map[this->pos3DtoIndex(x, y, z)] = VoxelType::Dirt;
					}
					else
					{
						map[this->pos3DtoIndex(x, y, z)] = VoxelType::Stone;
					}
				}
			}
		}
	}
}

VertexVector World::createVertexes( void )
{
	VertexVector vertexes;
	// NB add config to setup vertex generation optimization, i.e. face and frustum culling
	// NB add frustum culling

	for (ui32 z = 0U; z < this->worldSize.depth; z++)
	{
		for (ui32 x = 0U; x < this->worldSize.width; x++)
		{
			for (ui32 y = 0U; y < this->worldSize.height; y++)
			{
				VoxelType voxel = this->getVoxelType(x, y, z);
				if (voxel == VoxelType::Air) continue;
				
				vec3 globalPos = this->getRealWorldPos(x, y, z);
				std::map<VoxelFace,vec3> surroundings{
					std::pair<VoxelFace,vec3>(VoxelFace::LEFT, vec3{globalPos.x - 1.0f, globalPos.y, globalPos.z}),
					std::pair<VoxelFace,vec3>(VoxelFace::RIGHT, vec3{globalPos.x + 1.0f, globalPos.y, globalPos.z}),
					std::pair<VoxelFace,vec3>(VoxelFace::BACK, vec3{globalPos.x, globalPos.y, globalPos.z + 1.0f}),
					std::pair<VoxelFace,vec3>(VoxelFace::FRONT, vec3{globalPos.x, globalPos.y, globalPos.z - 1.0f}),
					std::pair<VoxelFace,vec3>(VoxelFace::BOTTOM, vec3{globalPos.x, globalPos.y - 1.0f, globalPos.z}),
					std::pair<VoxelFace,vec3>(VoxelFace::TOP, vec3{globalPos.x, globalPos.y + 1.0f, globalPos.z})
				};

				for (auto const& [faceDirection, position] : surroundings)
				{
					if (this->navigator.getVoxelType(position) == VoxelType::Air)
					{
						this->addFaceVertexes(vertexes, voxel, globalPos, faceDirection);
					}
				}
			}
		}
	}
	return vertexes;
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
	};
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

void World::addFaceVertexes( VertexVector& vertexes, VoxelType type, vec3 const& relativePos, VoxelFace face )
{
	VertexVector faceVertexes;

	switch (type)
	{
		case VoxelType::Dirt:
			faceVertexes = voxelFaceAtlasVertexes(relativePos, face);
			break;

		case VoxelType::Stone:
			faceVertexes = voxelFaceVertexes(relativePos, face);
			break;

		default:
			break;
	}

	vertexes.insert(vertexes.end(), faceVertexes.begin(), faceVertexes.end());
}

void World::addVoxelVertexes( VertexVector& vertexes, VoxelType type, vec3 const& relativePos )
{
	VertexVector voxelVertexess;

	switch (type)
	{
		case VoxelType::Dirt:
			voxelVertexess = voxelAtlasVertexes(relativePos);
			break;

		case VoxelType::Stone:
			voxelVertexess = voxelVertexes(relativePos);
			break;

		default:
			break;
	}

	vertexes.insert(vertexes.end(), voxelVertexess.begin(), voxelVertexess.end());
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

		this->generateVertexWorld(worldPos);
		while (this->getMemoryUsed() > this->maxVRAM)
		{
			this->dropWorld(this->findFurthestWorld());
		}
	}
}

VoxelType WorldNavigator::getVoxelType( vec3 const& globalPos ) const noexcept
{
	vec2i	worldIndex = this->getIndexWorld(globalPos);
	if ((this->doesWorldExist(worldIndex) == false) or (globalPos.y < 0.0f) or (globalPos.y >= this->worldSize.height))
	{
		return VoxelType::Air;
	}
	
	vec3ui	worldPos{
		positiveModulo(globalPos.x, this->worldSize.width),
		positiveModulo(globalPos.y, this->worldSize.height),
		positiveModulo(globalPos.z, this->worldSize.depth)
	};
	return this->worlds.at(worldIndex).getVoxelType(worldPos);
}

std::unique_ptr<ve::VulkanModel> WorldNavigator::createNewModel( ve::VulkanDevice& device, ui32 binding )
{
	std::vector<VertexVector*> vertexVector(this->vertexes.size());

	ui32 i = 0;
	for (auto& [_, vertexChunk] : this->vertexes)
	{
		vertexVector[i++] = &vertexChunk;
	} 
	std::unique_ptr<ve::VulkanModel> worldsModel = std::make_unique<ve::VulkanModel>(device, vertexVector, ve::MeshType::FACE, binding);
	this->updateModel = false;

	return worldsModel;
}

void WorldNavigator::addeNewWorld( vec2i const& worldIndex )
{
	assert(this->doesWorldExist(worldIndex) == false and "world already exists");

	this->worlds.emplace(worldIndex, World(worldIndex, this->worldSize, *this, this->seed));
	this->worlds.at(worldIndex).createMap();
}

void WorldNavigator::generateVertexWorld( vec2i const& worldIndex )
{
	assert(this->doesWorldExist(worldIndex) and "world doesn't exist");

	World& world = this->worlds.at(worldIndex);
	this->vertexes[worldIndex] = world.createVertexes();

	size_t nVertexes = this->vertexes[worldIndex].size();
	this->currentVRAM += nVertexes * sizeof(ve::VulkanModel::Vertex);
	this->currentVRAM += (nVertexes / ve::VERTEX_PER_FACE) * ve::INDEX_PER_FACE * sizeof(uint32_t);

	this->updateModel = true;
}

void WorldNavigator::dropWorld( vec2i const& worldToDropIndex )
{
	if (this->doesWorldExist(worldToDropIndex) == false) return;

	size_t nVertexes = this->vertexes[worldToDropIndex].size();
	this->currentVRAM -= nVertexes * sizeof(ve::VulkanModel::Vertex);
	this->currentVRAM -= (nVertexes / ve::VERTEX_PER_FACE) * ve::INDEX_PER_FACE * sizeof(uint32_t);

	this->worlds.erase(worldToDropIndex);
	this->vertexes.erase(worldToDropIndex);
}

vec2i WorldNavigator::findFurthestWorld( void ) noexcept
{
	vec2i furthestWorld = this->currentWorldPos;
	float furthestDist = 0.0f;
	for (auto const& [pos, world] : this->worlds)
	{
		float deltaSpace = vec2i::distance(pos, this->currentWorldPos);

		std::chrono::_V2::system_clock::time_point now = std::chrono::high_resolution_clock::now();		// NB use StopWatch
		uint32_t deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - world.getLastAccess()).count();

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
		static_cast<i32>(std::floor(globalPos.x / static_cast<float>(this->worldSize.width))),
		static_cast<i32>(std::floor(globalPos.z / static_cast<float>(this->worldSize.depth)))
	};
}

}	// namespace vox
