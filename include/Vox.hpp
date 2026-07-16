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

class Vox
{
	public:
		Vox( void );
		~Vox( void ) noexcept {};
		Vox( Vox const& ) = delete;
		Vox( Vox&& ) = delete;
		Vox& operator=( Vox const& ) = delete;
		Vox& operator=( Vox&& ) = delete;

		void run( void );

	private:
		void rotateCameraFromCursorPos( vec2 const& );
		void resizeWindow( ui32, ui32 );
		void toggleFullscreen( void );

		void setupVulkanBuffers( void );
		void setupVulkanDescSets( void );
		void setupVulkanPipelines( void );

		void moveCamera( float );
		void updateMap( std::future<bool>& mapUpdateResult );

		void updateUniforms(ui32 currentFrame);
		void drawTerrain(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawSkybox(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawText(VkCommandBuffer commandBuffer, ui32 currentFrame, std::string const& text);

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
		std::unique_ptr<ve::VulkanObject> textBackgroundObject;
		std::unique_ptr<ve::VulkanObject> fpsCounterObject;

		std::unique_ptr<ve::ViewProjectUniform> matrixUbo;
		std::unique_ptr<ve::MeshUniform>		materialsUbo;

		std::vector<std::unique_ptr<ve::VulkanDescriptorSet>>	uboDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> 				textureDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> 				fontDescriptorSet;

		std::unique_ptr<ve::VulkanPipeline> terrainPipeline;
		std::unique_ptr<ve::VulkanPipeline> skyboxPipeline;
		std::unique_ptr<ve::VulkanPipeline> fpsCounterPipeline;
	
		i32	countFramesToUpdate{0};

		bool walkFast{false};
};

}	// namespace vox
