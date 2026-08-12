#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Vulkan.hpp"
#include "Camera.hpp"
#include "Config.hpp"
#include "InputHandler.hpp"
#include "ThreadManager.hpp"
#include "World.hpp"
#include "Uniforms.hpp"


namespace vox {

class Vox
{
	public:
		Vox( void );
		~Vox( void ) noexcept = default;
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

		std::shared_ptr<ve::VulkanModel> createModel( ve::VertexVector const& vertexes, ve::IndexVector const& indexes, ui32 binding = 0U, ve::VertexLayout layout = ve::DEFAULT_MODEL_LAYOUT);

		void moveCamera( float );
		void updateMap( std::future<bool>& mapUpdateResult );

		void updateUniforms(ui32 currentFrame);
		void drawTerrain(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawSkybox(VkCommandBuffer commandBuffer, ui32 currentFrame);
		void drawUI(VkCommandBuffer commandBuffer, ui32 currentFrame, ui32 fps);

		ve::VulkanWindow				vulkanWindow;
		ve::VulkanDevice				vulkanDevice;
		ve::VulkanRenderer				vulkanRenderer;
		ve::VulkanDescriptorSetFactory	vulkanSetFactory;

		Camera			camera;
		WorldNavigator	navigator;
		InputHandler	inputHandler;
		ThreadManager	threadManager;

		std::unique_ptr<ve::VulkanObject> terrainObject;
		std::unique_ptr<ve::VulkanObject> caveObject;
		std::unique_ptr<ve::VulkanObject> skyboxObject;
		std::unique_ptr<ve::VulkanObject> backgroundUIObject;
		std::unique_ptr<ve::VulkanObject> textUIObject;

		std::unique_ptr<ViewProjectUniform> matrixUbo;
		std::unique_ptr<MeshUniform>		materialsUbo;
		std::unique_ptr<TextUniform>		textDataUbo;

		std::vector<std::unique_ptr<ve::VulkanDescriptorSet>>	uboDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> 				textureDescriptorSet;
		std::unique_ptr<ve::VulkanDescriptorSet> 				UIDescriptorSet;

		std::unique_ptr<ve::VulkanPipeline> terrainPipeline;
		std::unique_ptr<ve::VulkanPipeline> skyboxPipeline;
		std::unique_ptr<ve::VulkanPipeline> UIPipeline;
	
		i32	countFramesToUpdate{0};

		bool walkFast{false};
};

}	// namespace vox
