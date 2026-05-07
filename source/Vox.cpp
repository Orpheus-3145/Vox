#include "Vox.hpp"
#include "Stopwatch.hpp"
#include "Utils.hpp"
#include "World.hpp"

#include <iostream>
#include <cassert>
#include <future>

namespace vox {

/**
 * Create the engine of the game
 */
Vox::Vox( void ) :
	vulkanWindow{"ft_vox", Config::fullScreenMode, Config::defaultWindowWidth, Config::defaultWindowHeight},
	vulkanDevice{vulkanWindow},
	vulkanRenderer{vulkanWindow, vulkanDevice},
	vulkanSetFactory{vulkanDevice},
	camera{Config::cameraStartPos, Config::cameraForward.normalized(), this->vulkanWindow.getAspectRatio()},
	voxelMap{threadManager},
	inputHandler{
		[this](vec2 const& cursorPos) { this->rotateCameraFromCursorPos(cursorPos); },
		[this](i32 width, i32 height) { this->resizeWindow(width, height); },
		[this](void) { this->toggleFullscreen(); }
	}
{
	this->voxelMap.init();
	this->inputHandler.setCallbacks(this->vulkanWindow.getGLFWwindow());

	this->terrainObject = std::make_unique<ve::VulkanObject>();
	this->undergroundObject = std::make_unique<ve::VulkanObject>();
	this->skyboxObject = std::make_unique<ve::VulkanObject>();
}

void Vox::setupVulkan( void )
{
	this->terrainObject->setModel(this->voxelMap.createNewTerrainModel(vulkanDevice));
	this->undergroundObject->setModel(this->voxelMap.createNewUndergroundModel(vulkanDevice));
	this->skyboxObject->setModel(this->createVoxelMesh());

	this->matrixUbo = std::make_unique<ve::ViewProjectUniform>(this->camera.getViewMatrix(), this->camera.getProjectionMatrix());

	this->materialsUbo = std::make_unique<ve::MaterialUniform>();
	this->materialsUbo->updateMaterial(0U, Config::dirtMaterial);
	this->materialsUbo->updateLight(0U, Config::lightMaterial, this->camera.getViewMatrix(false));

	this->pipelineConstants = this->materialsUbo->getConstants();

	this->pushConstData = std::make_unique<ve::PushConstantsData>(
		this->terrainObject->getModelMatrix(),
		this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation())
	);

	ui32	maxSetsToCreate = 5;		// NB add those limits inside class VulkanDescriptorSet
	ui32	nUniformDescriptors = 2;
	ui32	nSamplerDescriptors = 3;

	this->vulkanSetFactory
		.setMaxSets(maxSetsToCreate)
		.setFramesInFlight(ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nUniformDescriptors)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nSamplerDescriptors)
		.createPool();

	ve::VulkanBindingSet uboSetBindings;
	uboSetBindings.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	uboSetBindings.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

	this->uboDescriptorSet = this->vulkanSetFactory.createDescriptorSet(uboSetBindings);
	this->uboDescriptorSet->addBufferDescriptor(0, sizeof(ve::ViewProjectUniform));
	this->uboDescriptorSet->addBufferDescriptor(1, sizeof(ve::MaterialUniform));

	ve::VulkanBindingSet textureTerrainSetBindings;
	textureTerrainSetBindings.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	this->textTerrainDescriptorSet = this->vulkanSetFactory.createDescriptorSet(textureTerrainSetBindings);
	this->textTerrainDescriptorSet->addSamplerDescriptor(0, Config::textureDirtPath, ve::TextureType::TEXTURE_PLAIN);

	ve::VulkanBindingSet textureUndergroundSetBindings;
	textureUndergroundSetBindings.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	this->textUndergroundDescriptorSet = this->vulkanSetFactory.createDescriptorSet(textureUndergroundSetBindings);
	this->textUndergroundDescriptorSet->addSamplerDescriptor(0, Config::textureStonePath, ve::TextureType::TEXTURE_PLAIN);

	ve::VulkanBindingSet textureSkyboxSetBindings;
	textureSkyboxSetBindings.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	this->textSkyboxDescriptorSet = this->vulkanSetFactory.createDescriptorSet(textureSkyboxSetBindings);
	this->textSkyboxDescriptorSet->addSamplerDescriptor(0, Config::textureSkyboxPath, ve::TextureType::TEXTURE_CUBEMAP);

	this->terrainObject->setModel(this->voxelMap.createNewTerrainModel(vulkanDevice));
	this->undergroundObject->setModel(this->voxelMap.createNewUndergroundModel(vulkanDevice));
	this->skyboxObject->setModel(this->createVoxelMesh());

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		this->uboDescriptorSet->getDescriptorSetLayout(),
		this->textTerrainDescriptorSet->getDescriptorSetLayout(),
		this->textUndergroundDescriptorSet->getDescriptorSetLayout(),
		this->textSkyboxDescriptorSet->getDescriptorSetLayout()
	};

	std::string vertexShader;
	std::string fragmentShader;
	if (Config::lightingMode == true)
	{
		vertexShader = Config::terrainVertShaderPath;
		fragmentShader = Config::terrainFragShaderPath;
	}
	else
	{
		vertexShader = Config::terrainNoLightVertShaderPath;
		fragmentShader = Config::terrainNoLightFragShaderPath;
	}

	this->terrainPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		descriptorSetLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		vertexShader,
		fragmentShader,
		this->terrainObject->getVboLayout(),
		false,
		sizeof(ve::PushConstantsData),
		&this->pipelineConstants
	);

	this->skyboxPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		descriptorSetLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		Config::skyboxVertShaderPath,
		Config::skyboxFragShaderPath,
		this->skyboxObject->getVboLayout(),
		true
	);

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

/**
 * Run the rendering loop
 */
void Vox::run( void )
{
	vec3 playerPos;
	Stopwatch timer;
 	float deltaTime = 0.0f;
	ui32 currentFrame = 0U;
	VkCommandBuffer commandBuffer = nullptr;
	std::future<bool>	mapUpdateResult;

	this->pushConstData->setMaterialIndex(0U);
	this->pushConstData->setLightIndex(0U);

	while (vulkanWindow.shouldClose() == false)
	{
		timer.start();
		glfwPollEvents();

		deltaTime = timer.elapsed(Unit::Seconds);
		this->moveCamera(deltaTime);

		vec3 playerPos = this->camera.getCameraPos();
		this->inputHandler.reset();

		if (mapUpdateResult.valid() == false)
		{
				mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
					return voxelMap.update(playerPos);
				});
		}
		else
		{
			const std::future_status status = mapUpdateResult.wait_for(std::chrono::milliseconds(0));

			if (status == std::future_status::ready)
			{
				const bool changed = mapUpdateResult.get(); // consumes future; now invalid

				if (changed == true)
				{
					this->terrainObject->setModel(this->voxelMap.createNewTerrainModel(vulkanDevice));
					this->undergroundObject->setModel(this->voxelMap.createNewUndergroundModel(vulkanDevice)); // main thread
				}
				mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
					return voxelMap.update(playerPos);
				});
			}
		}

		commandBuffer = this->vulkanRenderer.beginFrame();
		if (commandBuffer != nullptr)
		{
			this->vulkanRenderer.beginSwapChainRenderPass(commandBuffer);

			currentFrame = this->vulkanRenderer.getCurrentFrameIndex();
			this->uboDescriptorSet->setCurrentFrame(currentFrame);
			this->textTerrainDescriptorSet->setCurrentFrame(currentFrame);
			this->textUndergroundDescriptorSet->setCurrentFrame(currentFrame);
			this->textSkyboxDescriptorSet->setCurrentFrame(currentFrame);

			if (this->countFramesToUpdate > 0)
			{
				this->matrixUbo->updateView(this->camera.getViewMatrix());
				this->matrixUbo->updateProjection(this->camera.getProjectionMatrix());
				this->uboDescriptorSet->updateUbo(0, this->matrixUbo->getData());

				this->materialsUbo->updateLightDir(0, Config::lightDirection, this->camera.getViewMatrix(false));
				this->uboDescriptorSet->updateUbo(1, this->materialsUbo->getData());

				this->countFramesToUpdate--;
			}

			this->terrainPipeline->bindPipeline(commandBuffer);
			this->uboDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 0U);
			this->textTerrainDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 1U);
			this->terrainPipeline->updatePushConstants(commandBuffer, pushConstData->getData());

			this->terrainObject->bindBuffer(commandBuffer);
			this->terrainObject->draw(commandBuffer);

			this->textUndergroundDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 1U);
			this->undergroundObject->bindBuffer(commandBuffer);
			this->undergroundObject->draw(commandBuffer);

			this->skyboxPipeline->bindPipeline(commandBuffer);
			this->uboDescriptorSet->bindSet(commandBuffer, *this->skyboxPipeline, 0U);
			this->textSkyboxDescriptorSet->bindSet(commandBuffer, *this->skyboxPipeline, 1U);

			this->skyboxObject->bindBuffer(commandBuffer);
			this->skyboxObject->draw(commandBuffer);

			this->vulkanRenderer.endSwapChainRenderPass(commandBuffer);
			this->vulkanRenderer.endFrame();
		}
		timer.stop();

		// std::cout << "\033[K" << "Player position - x: " << playerPos.x << " y: " << playerPos.y << " z: " << playerPos.z << std::endl;
		// int32_t	fps = static_cast<int32_t> (1.0f / timer.elapsed(Unit::Seconds));
		// std::cout << "\033[3A" << "\033[K" << "Frames per second: " << fps << ", Frame time: " << timer.elapsed(Unit::Milliseconds) << "ms " << std::endl;
	}
	vkDeviceWaitIdle(vulkanDevice.device());
}

/**
 * Handle camera transformation in case of keys W-A-S-D or up-left-bottom-right (arrow) keys are pressed
 *
 * @param deltaTime to normalize the transformation, so that it doesn't depend on the fps
 * 
 * @note camera rotation using a key will be removed in the final version
 */
void Vox::moveCamera( float deltaTime )
{
	vec3	moveDirection = vec3::zero();
	vec3	rotation = vec3::zero();
	float	moveScalar = std::min(deltaTime * Config::movementSpeed, static_cast<float>(Config::chunkLength));
	float	rotationScalar = deltaTime * Config::lookSpeed;

	if (this->inputHandler.isKeyPressed(GLFW_KEY_W)) { moveDirection.z -= moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_S)) { moveDirection.z += moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_A)) { moveDirection.x -= moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_D)) { moveDirection.x += moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_Q)) { moveDirection.y -= moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_E)) { moveDirection.y += moveScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_UP)) { rotation.x += rotationScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_DOWN)) { rotation.x -= rotationScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_RIGHT)) { rotation.y += rotationScalar; }
	if (this->inputHandler.isKeyPressed(GLFW_KEY_LEFT))	{ rotation.y -= rotationScalar;	}

	if (rotation != vec3::zero())
	{
		this->camera.rotate(rotation.x, rotation.y, 0.0f);
		this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
	}
	if (moveDirection != vec3::zero())
	{
		// test for movement
		vec3 relativeMoveDirection = this->camera.getRelativeMoveDirection(moveDirection);
		vec3 location = this->camera.getCameraPos();

		vec3 movement = this->voxelMap.detectCollision(location, relativeMoveDirection);
		this->camera.move(movement);
		this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
	}
}

/**
 * Handle camera rotation my cursor movement
 *
 * @param newX x position (relative to the monitor) of the cursor ( (0;0): top-left corner)
 *
 * @param newY y position (relative to the monitor) of the cursor ( (0;0): top-left corner)
 *
 * @return a vector of 36 uin32_t starting from the offset value
 */
void	Vox::rotateCameraFromCursorPos( vec2 const& currPos )
{
	vec2 const& oldPos = this->inputHandler.getCursorPos();

	float yaw = (currPos.x - oldPos.x) * CameraSettings::cameraSensitivity;
	float pitch = (oldPos.y - currPos.y) * CameraSettings::cameraSensitivity;  // reversed since y-coordinates range from bottom to top
	this->camera.rotate(pitch, yaw, 0.0f);
	this->pushConstData->setNormalMatrix(this->camera.getViewMatrixNoTranslation());

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

/**
 * When a resize of the window happens, updates Vulkan and recalcolate projection matrix 
 * (since ration w/h changed)
 *
 * @param width new width
 *
 * @param height new height
 */
void Vox::resizeWindow( ui32 width, ui32 height )
{
	this->vulkanWindow.resetWindowSize(static_cast<i32>(width), static_cast<i32>(height));
	this->vulkanRenderer.recreateSwapChain();
	this->camera.updateAspect(this->vulkanWindow.getAspectRatio());
	this->pushConstData->setNormalMatrix(this->camera.getViewMatrixNoTranslation());

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::toggleFullscreen( void )
{
	this->vulkanWindow.toggleFullscreen();
	this->vulkanRenderer.recreateSwapChain();
	this->camera.updateAspect(this->vulkanWindow.getAspectRatio());
	this->pushConstData->setNormalMatrix(this->camera.getViewMatrixNoTranslation());

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}
/**
 * Creates a new ve::VulkanModel, that loads vertex data into the GPU. It shall be called everytime
 * a new world/chunks is created (i.e. whenever WorldNavigator::spawnCloseByWorlds() returns true)
 *
 * @param device vulkan object used to build the buffers
 *
 * @return pointer to the newly created model
 */
std::shared_ptr<ve::VulkanModel> Vox::createVoxelMesh( vec3 const& relativePos )
{
	return std::make_shared<ve::VulkanModel>(vulkanDevice, getVertexAtlasRelative(relativePos), getIndexRelative());
}

}	// namespace vox
