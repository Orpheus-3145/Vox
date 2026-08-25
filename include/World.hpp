#pragma once

#include <array>
#include <unordered_map>
#include <optional>

#include "Vulkan.hpp"
#include "NoiseGenerator.hpp"
#include "Stopwatch.hpp"
#include "Camera.hpp"
#include "ThreadManager.hpp"


namespace vox {

enum class VoxelType : ui8
{
	Air = 0,
	Dirt = 1,
	Stone = 2,
};

enum VoxelFace : ui8
{
	FRONT = 0,
	BACK = 4,
	LEFT = 8,
	RIGHT = 12,
	TOP = 16,
	BOTTOM = 20
};

inline constexpr float VOXEL_SIZE = 1.0f;
static_assert( VOXEL_SIZE > 0.0f and "Invalid Voxel size provided" );

inline constexpr size_t	VERTEX_PER_VOXEL = 24U;	// number of vertexes per voxe
inline constexpr size_t	INDEX_PER_VOXEL = 36U;	// number of vertex indexes per voxel
inline constexpr size_t	VERTEX_PER_FACE = 4U;	// number of vertexes per face (of a voxel)
inline constexpr size_t	INDEX_PER_FACE = 6U;	// number of vertex indexes per voxel

// Hard-coded VBO (vertex+normal+textureUV data) of a voxel (standard texture coordinates)
inline constexpr std::array<ve::Vertex,VERTEX_PER_VOXEL> VOXEL_VERTEXES{
	// front
	ve::Vertex{vec3{ 0.0f,       0.0f,       0.0f }, vec3::forward(), vec2{ 0.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       0.0f }, vec3::forward(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::forward(), vec2{ 1.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, 0.0f }, vec3::forward(), vec2{ 0.0f, 1.0f }},
	// back
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       VOXEL_SIZE }, vec3::backward(), vec2{ 0.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f,       0.0f,       VOXEL_SIZE }, vec3::backward(), vec2{ 1.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, VOXEL_SIZE }, vec3::backward(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::backward(), vec2{ 0.0f, 0.0f }},
	// left
	ve::Vertex{vec3{ 0.0f, 0.0f,       0.0f }, vec3::left(), vec2{ 0.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f, VOXEL_SIZE, 0.0f }, vec3::left(), vec2{ 0.0f, 0.0f }},
	ve::Vertex{vec3{ 0.0f, VOXEL_SIZE, VOXEL_SIZE }, vec3::left(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ 0.0f, 0.0f,       VOXEL_SIZE }, vec3::left(), vec2{ 1.0f, 1.0f }},
	// right
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       VOXEL_SIZE }, vec3::right(), vec2{ 0.0f, 1.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::right(), vec2{ 0.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::right(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       0.0f }, vec3::right(), vec2{ 1.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, VOXEL_SIZE }, vec3::up(), vec2{ 0.0f, 1.0f }},
	// up
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, 0.0f }, vec3::up(), vec2{ 0.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::up(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::up(), vec2{ 1.0f, 1.0f }},
	// down
	ve::Vertex{vec3{ 0.0f,       0.0f, 0.0f }, vec3::down(), vec2{ 0.0f, 1.0f }},
	ve::Vertex{vec3{ 0.0f,       0.0f, VOXEL_SIZE }, vec3::down(), vec2{ 0.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f, VOXEL_SIZE }, vec3::down(), vec2{ 1.0f, 0.0f }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f, 0.0f }, vec3::down(), vec2{ 1.0f, 1.0f }}
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
	ve::Vertex{vec3{ 0.0f,       0.0f,       0.0f }, vec3::forward(), vec2{ W + padding, padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       0.0f }, vec3::forward(), vec2{ 2 * W - padding, padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::forward(), vec2{ 2 * W - padding, H - padding }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, 0.0f }, vec3::forward(), vec2{ W + padding, H - padding }},
	// BACK
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       VOXEL_SIZE }, vec3::backward(), vec2{ 2 * W - padding, padding }},
	ve::Vertex{vec3{ 0.0f,       0.0f,       VOXEL_SIZE }, vec3::backward(), vec2{ W + padding, padding }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, VOXEL_SIZE }, vec3::backward(), vec2{ W + padding, H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::backward(), vec2{ 2 * W - padding, H - padding }},
	// LEFT
	ve::Vertex{vec3{ 0.0f, 0.0f,       0.0f }, vec3::left(), vec2{ padding, H + padding }},
	ve::Vertex{vec3{ 0.0f, VOXEL_SIZE, 0.0f }, vec3::left(), vec2{ W - padding, H + padding }},
	ve::Vertex{vec3{ 0.0f, VOXEL_SIZE, VOXEL_SIZE }, vec3::left(), vec2{ W - padding, 2 * H - padding }},
	ve::Vertex{vec3{ 0.0f, 0.0f,       VOXEL_SIZE }, vec3::left(), vec2{ padding, 2 * H - padding }},
	// RIGHT
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       VOXEL_SIZE }, vec3::right(), vec2{ 3 * W - padding, 2 * H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::right(), vec2{ 2 * W + padding, 2 * H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::right(), vec2{ 2 * W + padding, H + padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f,       0.0f }, vec3::right(), vec2{ 3 * W - padding, H + padding }},
	// TOP
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, VOXEL_SIZE }, vec3::up(), vec2{ W + padding, 2 * H - padding }},
	ve::Vertex{vec3{ 0.0f,       VOXEL_SIZE, 0.0f }, vec3::up(), vec2{ 2 * W - padding, 2 * H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, 0.0f }, vec3::up(), vec2{ 2 * W - padding, H + padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, vec3::up(), vec2{ W + padding, H + padding }},
	// BOTTOM
	ve::Vertex{vec3{ 0.0f,       0.0f, 0.0f }, vec3::down(), vec2{ 3 * W + padding, H + padding }},
	ve::Vertex{vec3{ 0.0f,       0.0f, VOXEL_SIZE }, vec3::down(), vec2{ 3 * W + padding, 2 * H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f, VOXEL_SIZE }, vec3::down(), vec2{ 4 * W - padding, 2 * H - padding }},
	ve::Vertex{vec3{ VOXEL_SIZE, 0.0f, 0.0f }, vec3::down(), vec2{ 4 * W - padding, H + padding }}
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
		explicit World( vec2i const& indexWorld, vec3ui const& worldSize, WorldNavigator& navigator, NoiseGenerator const& generator );
		World( void ) = delete;
		~World( void ) noexcept = default;
		World( World const& other ) = delete;
		World( World&& other ) = default;
		World& operator=( World const& other ) = delete;
		World& operator=( World&& other ) = delete;

		void				drawTerrain( VkCommandBuffer commandBuffer ) const;
		void				drawCave( VkCommandBuffer commandBuffer ) const;

		void 				createMap( void );
		ve::VertexVector	createTerrainVertexes( bool applyFaceCulling ) const;
		ve::VertexVector	createCaveVertexes( bool applyFaceCulling ) const;
		void				createTerrainVertexes( bool applyFaceCulling, ve::VertexVector& vertexes ) const;
		void				createCaveVertexes( bool applyFaceCulling, ve::VertexVector& vertexes ) const;
		void				createTerrainVertexesMT( bool applyFaceCulling, ve::VulkanDevice& vulkanDevice );
		void				createCaveVertexesMT( bool applyFaceCulling, ve::VulkanDevice& vulkanDevice );

		VoxelType	getVoxelType( vec3ui const& index ) const;
		VoxelType	getVoxelType( ui32 x, ui32 y, ui32 z ) const;
		vec3		getRealWorldPos( ui32 x, ui32 y, ui32 z ) const noexcept;
		vec3		getRealWorldPos( vec3ui const& worldPos ) const noexcept;
		size_t		getVRAMsize( void ) const noexcept { return this->VRAMsize; }

		void	setLastAccess( void ) noexcept { this->lastAccess.start(); }
		float	getLastAccess( void ) const noexcept { return this->lastAccess.getTime(Unit::Milliseconds); }

		ui32	pos3DtoIndex( ui32 x, ui32 y, ui32 z ) const noexcept;
		ui32	pos3DtoIndex( vec3ui const& pos ) const noexcept;
		vec3ui	indexToPos3D( ui32 index ) const noexcept;

	private:
		vec2i const				indexWorld;
		vec3ui const			worldSize;
		size_t					VRAMsize{0UL};

		WorldNavigator const&	navigator;
		NoiseGenerator const&	generator;

		std::vector<VoxelType>	map;
		ve::VulkanObject		terrainObject;
		ve::VulkanObject		caveObject;

		Stopwatch	lastAccess;
};

class WorldNavigator {
	public:
		explicit WorldNavigator( ve::VulkanDevice& vulkanDevice, uint32_t worldLength, uint32_t worldHeight, size_t maxVRAM, ui32 seed ) :
			vulkanDevice{vulkanDevice},
			worldSize{worldLength, worldHeight, worldLength},
			maxVRAM{maxVRAM},
			generator{seed} {}
		WorldNavigator( void ) = delete;
		~WorldNavigator( void ) = default;
		WorldNavigator( WorldNavigator const& other ) = delete;
		WorldNavigator( WorldNavigator&& other ) = delete;
		WorldNavigator& operator=( WorldNavigator const& other ) = delete;
		WorldNavigator& operator=( WorldNavigator&& other ) = delete;

		void		spawnCloseByWorlds( vec3 const& start );
		void		spawnCloseByWorldsMT( vec3 const& start );
		VoxelType	getVoxelType( vec3 const& globalPos ) const noexcept;
		size_t		getMemoryUsed( void ) const noexcept { return this->currentVRAM; };
		bool		borderCrossed( vec3 const& currentPos ) const noexcept { return this->currentWorldPos != this->getIndexWorld(currentPos); }
		bool		doesWorldExist( vec2i const& checkPos) const noexcept { return this->worlds.find(checkPos) != this->worlds.end(); }
		vec3		checkClipping( vec3 const& startPos, vec3 const& endPos ) const noexcept;
		bool		isReady( void ) const noexcept { return this->worldReady; }

		void	drawTerrain( VkCommandBuffer commandBuffer, std::optional<FrustumBox> const& frustum = std::nullopt ) const noexcept;
		void	drawCaves( VkCommandBuffer commandBuffer, std::optional<FrustumBox> const& frustum = std::nullopt ) const noexcept;

		static constexpr float ALPHA = 0.8f;	// weight for distance
		static constexpr float BETA = 0.2f;		// weight for delta time

		static constexpr float STEP_SIZE = VOXEL_SIZE / 10;		// steps that checks the clipping collision
		static constexpr float RADIUS = VOXEL_SIZE / 3;			// radius of the player before touching walls

	private:
		void	addeNewWorld( vec2i const& worldIndex );
		void	generateModelWorld( vec2i const& worldIndex );
		void	generateModelWorld( vec2i const& worldIndex, ve::VertexVector const& terrainVertexes, ve::VertexVector const& caveVertexes );
		void	dropWorld( vec2i const& worldIndex );
		bool	checkCollisionRadius( vec3 const& position) const noexcept;

		vec2i	findFurthestWorld( void ) const noexcept;
		vec2i	getIndexWorld( vec3 const& globalPos ) const noexcept;

		bool	isWorldVisible(vec2i const& worldIndex, FrustumBox const& frustum) const;

		ve::VulkanDevice&	vulkanDevice;
		vec3ui const		worldSize;
		size_t const		maxVRAM;

		NoiseGenerator	generator;
		ThreadManager	orchestrator{};

		vec2i	currentWorldPos{-1000};
		size_t	currentVRAM{0UL};

		bool	applyFaceCulling{true};
		bool	applyFrustumCulling{true};
		bool	worldReady{false};

		std::unordered_map<vec2i,World>				worlds;
		std::unordered_map<vec2i,ve::VulkanObject>	terrain;
		std::unordered_map<vec2i,ve::VulkanObject>	cave;
};

}	// namespace vox
