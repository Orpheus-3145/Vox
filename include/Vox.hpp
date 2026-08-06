#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Vulkan.hpp"
#include "Camera.hpp"
#include "Config.hpp"
#include "InputHandler.hpp"
#include "ThreadManager.hpp"
#include "VoxelMap.hpp"
#include "World.hpp"
#include "Uniforms.hpp"


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

		std::unique_ptr<ve::VulkanModel> createSkyboxModel( void );
		void moveCamera( float );
		void updateMap( std::future<bool>& mapUpdateResult );

		void updateUniforms(ui32 currentFrame);
		void drawTerrain(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawSkybox(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawTextFPS(VkCommandBuffer commandBuffer, ui32 currentFrame, std::string const& text, vec2i const& position);
		void drawTextMemory(VkCommandBuffer commandBuffer, ui32 currentFrame, std::string const& text, vec2i const& position);

		ve::VulkanWindow				vulkanWindow;
		ve::VulkanDevice				vulkanDevice;
		ve::VulkanRenderer				vulkanRenderer;
		ve::VulkanDescriptorSetFactory	vulkanSetFactory;

		Camera			camera;
		WorldNavigator	navigator;
		InputHandler	inputHandler;
		ThreadManager	threadManager;
		
		std::unique_ptr<ve::VulkanObject> terrainObject;
		std::unique_ptr<ve::VulkanObject> undergroundObject;
		std::unique_ptr<ve::VulkanObject> skyboxObject;
		std::unique_ptr<ve::VulkanObject> fpsBackgroundObject;
		std::unique_ptr<ve::VulkanObject> fpsTextObject;
		std::unique_ptr<ve::VulkanObject> memoryBackgroundObject;
		std::unique_ptr<ve::VulkanObject> memoryTextObject;

		std::unique_ptr<ViewProjectUniform> matrixUbo;
		std::unique_ptr<MeshUniform>		materialsUbo;
		std::unique_ptr<TextUniform>		textDataUbo;

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
