#pragma once

#include <array>
#include <unordered_map>

#include "Vulkan.hpp"
#include "NoiseGenerator.hpp"


namespace vox {

enum class VoxelType : ui8
{
	Air = 0,
	Dirt = 1,
	Stone = 2,
	Water = 3,		// NB remove it
	Padding = 255		// NB remove it
};

enum VoxelFace : size_t		// NB set it as ui8
{
	FRONT = 0,
	BACK = 4,
	LEFT = 8,
	RIGHT = 12,
	TOP = 16,
	BOTTOM = 20
};

enum class Direction : ui8		// NB remove it
{
	North,
	East,
	South,
	West
};

inline constexpr size_t	VERTEX_PER_VOXEL = 24U;	// number of vertexes per voxe
inline constexpr size_t	INDEX_PER_VOXEL = 36U;	// number of vertex indexes per voxel
inline constexpr size_t	VERTEX_PER_FACE = 4U;	// number of vertexes per face (of a voxel)
inline constexpr size_t	INDEX_PER_FACE = 6U;	// number of vertex indexes per voxel

// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (standard texture coordinates)
inline constexpr std::array<ve::Vertex,VERTEX_PER_VOXEL> VOXEL_VERTEXES{
	// FRONT
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::forward(), vec2{ 0.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::forward(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::forward(), vec2{ 1.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::forward(), vec2{ 0.0f, 1.0f }, 1U},
	// BACK
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::backward(), vec2{ 0.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::backward(), vec2{ 1.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::backward(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::backward(), vec2{ 0.0f, 0.0f }, 1U},
	// LEFT
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::left(), vec2{ 0.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::left(), vec2{ 0.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::left(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::left(), vec2{ 1.0f, 1.0f }, 1U},
	// RIGHT
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::right(), vec2{ 0.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::right(), vec2{ 0.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::right(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::right(), vec2{ 1.0f, 1.0f }, 1U},
	// TOP
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::up(), vec2{ 0.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 0.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::up(), vec2{ 1.0f, 1.0f }, 1U},
	// BOTTOM
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 0.0f, 1.0f }, 1U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 0.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 1.0f, 0.0f }, 1U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 1.0f, 1.0f }, 1U}
};

// it assumes the cubemap has this shape
//  ___ ___ ___ ___ 
// |   |Bac|   |   |
// |___|___|___|___|
// | L | T | R |Bot|
// |___|___|___|___|
// |   | F |   |   |
// |___|___|___|___|
static constexpr float W = 1.0f / 4.0f;  // width of a tile
static constexpr float H = 1.0f / 3.0f;  // height of a tile
static constexpr float padding = 0.004f;
// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (atlas texture coordinates)
inline constexpr std::array<ve::Vertex,VERTEX_PER_VOXEL> VOXEL_VERTEXES_ATLAS{
	// FRONT
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::forward(), vec2{ W + padding, padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::forward(), vec2{ 2 * W - padding, padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::forward(), vec2{ 2 * W - padding, H - padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::forward(), vec2{ W + padding, H - padding }, 0U},
	// BACK
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::backward(), vec2{ 2 * W - padding, padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::backward(), vec2{ W + padding, padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::backward(), vec2{ W + padding, H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::backward(), vec2{ 2 * W - padding, H - padding }, 0U},
	// LEFT
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::left(), vec2{ padding, H + padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::left(), vec2{ W - padding, H + padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::left(), vec2{ W - padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::left(), vec2{ padding, 2 * H - padding }, 0U},
	// RIGHT
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::right(), vec2{ 3 * W - padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::right(), vec2{ 2 * W + padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::right(), vec2{ 2 * W + padding, H + padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::right(), vec2{ 3 * W - padding, H + padding }, 0U},
	// TOP
	ve::Vertex{vec3{ 0.0f, 1.0f, 1.0f }, vec3::up(), vec2{ W + padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 2 * W - padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 0.0f }, vec3::up(), vec2{ 2 * W - padding, H + padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 1.0f, 1.0f }, vec3::up(), vec2{ W + padding, H + padding }, 0U},
	// BOTTOM
	ve::Vertex{vec3{ 0.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 3 * W + padding, H + padding }, 0U},
	ve::Vertex{vec3{ 0.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 3 * W + padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 1.0f }, vec3::down(), vec2{ 4 * W - padding, 2 * H - padding }, 0U},
	ve::Vertex{vec3{ 1.0f, 0.0f, 0.0f }, vec3::down(), vec2{ 4 * W - padding, H + padding }, 0U}
};

// hard-coded indexes of a voxel
inline constexpr std::array<ui32, INDEX_PER_VOXEL> VOXEL_INDEXES{
	0U, 2U, 1U, 		// front face
	0U, 3U, 2U, 		// front face
	4U, 6U, 5U, 		// back face
	4U, 7U, 6U, 		// back face
	8U, 10U, 9U, 		// left face
	8U, 11U, 10U, 		// left face
	12U, 14U, 13U, 		// right face
	12U, 15U, 14U, 		// right face
	16U, 18U, 17U, 		// top face
	16U, 19U, 18U, 		// top face
	20U, 22U, 21U, 		// bottom face
	20U, 23U, 22U		// bottom face
};

// hard-coded indexes of a face
inline constexpr std::array<ui32, INDEX_PER_FACE> FACE_INDEXES{
	0U, 2U, 1U, 		// front face
	0U, 3U, 2U	 		// front face
};

ve::VertexVector	voxelVertexes( vec3 const& relativeOrigin = vec3(0.0f) );
ve::VertexVector	voxelAtlasVertexes( vec3 const& relativeOrigin = vec3(0.0f) );
ve::VertexVector	voxelFaceVertexes( vec3 const& relativeOrigin = vec3(0.0f), VoxelFace face = VoxelFace::FRONT );
ve::VertexVector	voxelFaceAtlasVertexes( vec3 const& relativeOrigin = vec3(0.0f), VoxelFace face = VoxelFace::FRONT );

ve::IndexVector		voxelIndexes( ui32 start = 0U );
ve::IndexVector		voxelFaceIndexes( ui32 start = 0U );


class WorldNavigator;

class World {
	public:
		explicit World( vec2i const& indexWorld, vec3ui const& worldSize, WorldNavigator& navigator, ui32 seed );
		World( void ) = delete;
		~World( void ) noexcept = default;
		World( World const& other ) = delete;
		World( World&& other ) = default;
		World& operator=( World const& other ) = delete;
		World& operator=( World&& other ) = delete;

		void 				createMap( void );
		ve::VertexVector	createVertexes( void );

		VoxelType		getVoxelType( vec3ui const& index ) const;
		VoxelType		getVoxelType( ui32 x, ui32 y, ui32 z ) const;
		vec3			getRealWorldPos( ui32 x, ui32 y, ui32 z ) const noexcept;
		vec3			getRealWorldPos( vec3ui const& worldPos ) const noexcept;
		void												setLastAccess( void ) noexcept { this->lastAccess = std::chrono::high_resolution_clock::now(); }
		std::chrono::_V2::system_clock::time_point const&	getLastAccess( void ) const noexcept { return this->lastAccess; }

		ui32		pos3DtoIndex( ui32 x, ui32 y, ui32 z ) const noexcept;
		ui32		pos3DtoIndex( vec3ui const& pos ) const noexcept;
		vec3ui		indexToPos3D( ui32 index ) const noexcept;

	private:
		vec2i const				indexWorld;
		vec3ui const			worldSize;

		WorldNavigator&			navigator;
		NoiseGenerator			generator;

		std::vector<VoxelType>	map;

		std::chrono::_V2::system_clock::time_point	lastAccess;		// NB use StopWatch
};

class WorldNavigator {
	public:
		explicit WorldNavigator( uint32_t worldLength, uint32_t worldHeight, size_t maxVRAM, ui32 seed ) :
			worldSize{worldLength, worldHeight, worldLength}, maxVRAM{maxVRAM}, seed{seed} {}
		WorldNavigator( void ) = delete;
		~WorldNavigator( void ) = default;
		WorldNavigator( WorldNavigator const& other ) = delete;
		WorldNavigator( WorldNavigator&& other ) = delete;
		WorldNavigator& operator=( WorldNavigator const& other ) = delete;
		WorldNavigator& operator=( WorldNavigator&& other ) = delete;

		void		spawnCloseByWorlds( vec3 const& start );
		VoxelType	getVoxelType( vec3 const& globalPos ) const noexcept;
		size_t		getMemoryUsed( void ) const noexcept;
		bool		spawnNewModel( void ) const noexcept { return this->updateModel; }
		bool		borderCrossed( vec3 const& currentPos ) const noexcept { return this->currentWorldPos != this->getIndexWorld(currentPos); }
		bool		doesWorldExist( vec2i const& checkPos) const noexcept { return this->worlds.find(checkPos) != this->worlds.end(); }

		std::unique_ptr<ve::VulkanModel>	createNewModel( ve::VulkanDevice& device, ui32 binding = 0U );

		static constexpr float ALPHA = 0.8f;	// weight for distance
		static constexpr float BETA = 0.2f;		// weight for delta time

	private:
		void	addeNewWorld( vec2i const& worldIndex );
		void	generateVertexWorld( vec2i const& worldIndex );
		void	dropWorld( vec2i const& worldIndex );
		vec2i	findFurthestWorld( void ) noexcept;
		vec2i	getIndexWorld( vec3 const& globalPos ) const noexcept;

		vec3ui const	worldSize;
		size_t const	maxVRAM;
		ui32 const		seed;

		vec2i	currentWorldPos{-1000};
		bool	updateModel{false};
		size_t	nFaces{0UL};
		
		std::unordered_map<vec2i,World>				worlds;
		std::unordered_map<vec2i,ve::VertexVector>	vertexes;
};

}	// namespace vox
