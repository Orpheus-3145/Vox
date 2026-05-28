#pragma once

#include "Vulkan.hpp"
#include "Camera.hpp"
#include "Config.hpp"
#include "InputHandler.hpp"
#include "ThreadManager.hpp"
#include "VoxelMap.hpp"
#include "TypeAliases.hpp"

#include <array>
#include <memory>
#include <vector>


namespace vox {

// Vulkan variables layout for shaders is 16b so each member size has to multiple of 16 or padded
class ViewProjectUBO
{
	public:
		ViewProjectUBO( void ) = delete;
		ViewProjectUBO( mat4 const& view, mat4 const& projection ) :
			view{view},
			projection{projection} {};

		void	updateView( mat4 const& view ) noexcept { this->view = view; };
		void	updateProjection( mat4 const& prj ) noexcept { this->projection = prj; };

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); };

	private:
		mat4 view;
		mat4 projection;
};

// Vulkan variables layout for shaders is 16b so each member size has to multiple of 16 or padded
class LightUBO
{
	public:
		LightUBO( void ) = delete;
		LightUBO( vec3 const& lightDir, mat4 const& viewMatrix, vec3 const& ambientColor, vec3 const& diffuseColor, vec3 const& specularColor ) :	// , vec3 const& viewPos
			lightAmbientColor{ambientColor, 1.0f},
			lightColor{diffuseColor, 1.0f},
			lightSpecularColor{specularColor, 1.0f} {
				this->updateLightDir(lightDir, viewMatrix);
			};

		void	updateLightDir( vec3 const& lightDir, mat4 const& viewMatrix ) noexcept;

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); };

	private:
		vec4 	lightDir;
		vec4	lightAmbientColor;
		vec4	lightColor;
		vec4	lightSpecularColor;
};


class MeshData
{
	public:
		MeshData( void ) = delete;
		MeshData( mat4 const& modelMatrix, mat4 const& normalMatrix, ve::MeshMaterial const& material ) :
			modelMatrix{modelMatrix},
			normalMatrix{normalMatrix},
			material{material} {};
		MeshData( MeshData const& other ) = default;
		MeshData( MeshData&& other ) = default;
		MeshData& operator=( MeshData const& other ) = default;
		MeshData& operator=( MeshData&& other ) = default;

		void	updateModelMatrix( mat4 const& modelMatrix ) noexcept { this->modelMatrix = modelMatrix; };
		void	updateNormalMatrix( mat4 const& normalMatrix ) noexcept { this->normalMatrix = normalMatrix; };
		void	updateMaterial( ve::MeshMaterial const& material ) noexcept { this->material = material; };

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); };

	private:
		mat4				modelMatrix{1.0f};
		mat4				normalMatrix{1.0f};
		ve::MeshMaterial	material{};
};

// vulkan push_constant max size should be 128 or 256 b
static_assert(sizeof(MeshData) == 192);
static_assert(sizeof(MeshData) == 2 * sizeof(mat4) + sizeof(ve::MeshMaterial));


class Vox
{
	public:
		Vox( void );
		~Vox( void ) noexcept {};
		Vox( Vox const& ) = delete;
		Vox( Vox&& ) = delete;
		Vox& operator=( Vox const& ) = delete;
		Vox& operator=( Vox&& ) = delete;

		void setupVulkan( void );
		void run( void );

		void moveCamera( float );
		void rotateCameraFromCursorPos( vec2 const& );
		void resizeWindow( ui32, ui32 );
		void toggleFullscreen( void );

		
		private:
		std::shared_ptr<ve::VulkanModel> createVoxelMesh( vec3 const& = vec3{-0.5f, -0.5f, -0.5f} );
		
		ve::VulkanWindow				vulkanWindow;
		ve::VulkanDevice				vulkanDevice;
		ve::VulkanRenderer				vulkanRenderer;
		ve::VulkanDescriptorSetFactory	vulkanSetFactory;
		
		Camera			camera;
		VoxelMap		voxelMap;
		InputHandler	inputHandler;
		ThreadManager	threadManager;
		
		std::unique_ptr<ve::VulkanObject> terrainObject;
		std::unique_ptr<ve::VulkanObject> undergroundObject;
		std::unique_ptr<ve::VulkanObject> skyboxObject;
		
		std::unique_ptr<ve::VulkanDescriptorSet> uboDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> textTerrainDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> textUndergroundDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> textSkyboxDescriptorSet;
		
		std::unique_ptr<ve::VulkanPipeline> terrainPipeline;
		std::unique_ptr<ve::VulkanPipeline> skyboxPipeline;
		
		// since there's a copy of every descriptor for every frame in flight,
		// this flag is to update each ubo in a set, for every frame
		i32	countFramesToUpdate{0};
};

}	// namespace vox
