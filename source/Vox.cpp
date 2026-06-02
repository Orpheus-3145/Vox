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
	this->skyboxObject->setModel(this->createVoxelMesh());		// NB use only vertex data, no normal, uv, ...

	this->matrixUbo = std::make_unique<ve::ViewProjectUniform>(this->camera.getViewMatrix(), this->camera.getProjectionMatrix());

	this->materialsUbo = std::make_unique<ve::MeshUniform>();
	this->materialsUbo->updateModelMatrix(0, this->terrainObject->getModelMatrix());
	this->materialsUbo->updateModelMatrix(1, this->undergroundObject->getModelMatrix());
	this->materialsUbo->updateModelMatrix(2, mat4::idMat());

	this->materialsUbo->updateNormalMatrix(0, this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateNormalMatrix(1, this->undergroundObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateNormalMatrix(2, mat4::idMat());

	this->materialsUbo->updateMaterial(0U, Config::dirtMaterial);
	this->materialsUbo->updateMaterial(1U, Config::stoneMaterial);
	this->materialsUbo->updateLight(0U, Config::lightMaterial, this->camera.getViewMatrix(false));

	ui32	maxSetsToCreate = 1U * ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT + 1U;
	ui32	nUniformDescriptors = 2U * ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
	ui32	nSamplerDescriptors = 5U;

	this->vulkanSetFactory
		.setMaxSets(maxSetsToCreate)
		.addBufferPoolSize(nUniformDescriptors)
		.addSamplerPoolSize(nSamplerDescriptors)
		.createPool();

	// NB create a second set to store material data instead of using the second binding
	// NB use set = 0 binding 0 for view, binding 1 for projection ?
	ve::VulkanBindingSet uboSetBindings;
	uboSetBindings.addBufferBinding(0, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ve::ViewProjectUniform));
	uboSetBindings.addBufferBinding(1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ve::MeshUniform));

	this->terrainObject->setModel(this->voxelMap.createNewTerrainModel(vulkanDevice));
	this->undergroundObject->setModel(this->voxelMap.createNewUndergroundModel(vulkanDevice));
	this->skyboxObject->setModel(this->createVoxelMesh());

	this->uboDescriptorSet.resize(ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0U; i < ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		this->uboDescriptorSet[i] = this->vulkanSetFactory.createDescriptorSet(uboSetBindings);
	}

	std::vector<std::string>	texturePaths{
		Config::textureDirt1,
		Config::textureDirt2,
		Config::textureStone1,
		Config::textureStone2
	};
	std::vector<ve::TextureType>	textureTypes{
		ve::TextureType::TEXTURE_PLAIN,
		ve::TextureType::TEXTURE_PLAIN,
		ve::TextureType::TEXTURE_PLAIN,
		ve::TextureType::TEXTURE_PLAIN
	};

	ve::VulkanBindingSet textureSetBindings;
	textureSetBindings.addSamplerArrayBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, texturePaths, textureTypes);
	textureSetBindings.addSamplerBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, Config::textureSkybox, ve::TextureType::TEXTURE_CUBEMAP);
	this->textureDescriptorSet = this->vulkanSetFactory.createDescriptorSet(textureSetBindings);

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
		this->vulkanSetFactory.getDescriptorSetLayout(),
		this->vulkanRenderer.getSwapChainRenderPass(),
		vertexShader,
		fragmentShader,
		this->terrainObject->getVboLayout(),
		false,
		sizeof(DrawDataLimit),
		&ve::drawingDataLimits
	);

	this->skyboxPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		this->vulkanSetFactory.getDescriptorSetLayout(),
		this->vulkanRenderer.getSwapChainRenderPass(),
		Config::skyboxVertShaderPath,
		Config::skyboxFragShaderPath,
		this->skyboxObject->getVboLayout(),
		true,
		sizeof(DrawDataLimit),
		&ve::drawingDataLimits
	);

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

/**
 * Run the rendering loop
 */
void Vox::run( void )
{
	vec3				playerPos;
	Stopwatch			timer;
 	float				deltaTime = 0.0f;
	ui32				currentFrame = 0U;

	VkCommandBuffer		commandBuffer = nullptr;
	std::future<bool>	mapUpdateResult;
	DrawDataIndex		indexes{};

	while (vulkanWindow.shouldClose() == false)
	{
		timer.start();
		glfwPollEvents();

		deltaTime = timer.elapsed(Unit::Seconds);
		this->moveCamera(deltaTime);

		playerPos = this->camera.getCameraPos();
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

			if (this->countFramesToUpdate > 0)
			{
				this->matrixUbo->updateView(this->camera.getViewMatrix());
				this->matrixUbo->updateProjection(this->camera.getProjectionMatrix());
				this->uboDescriptorSet[currentFrame]->updateDescriptor(0, this->matrixUbo->getData());

				this->materialsUbo->updateNormalMatrix(0, this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
				this->materialsUbo->updateNormalMatrix(1, this->undergroundObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
				this->materialsUbo->updateLightDir(0, Config::lightDirection, this->camera.getViewMatrix(false));
				this->uboDescriptorSet[currentFrame]->updateDescriptor(1, this->materialsUbo->getData());

				this->countFramesToUpdate--;
			}

			this->terrainPipeline->bindPipeline(commandBuffer);

			this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->terrainPipeline, 0U);
			this->textureDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 1U);

			indexes.models = 0U;
			indexes.materials = 0U;
			indexes.textures = 0U;
			this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

			this->terrainObject->bindBuffer(commandBuffer);
			this->terrainObject->draw(commandBuffer);

			indexes.models = 1U;
			indexes.materials = 1U;
			indexes.textures = 2U;
			this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

			this->undergroundObject->bindBuffer(commandBuffer);
			this->undergroundObject->draw(commandBuffer);

			this->skyboxPipeline->bindPipeline(commandBuffer);

			indexes.models = 2U;
			this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

			this->skyboxObject->bindBuffer(commandBuffer);
			this->skyboxObject->draw(commandBuffer);

			this->vulkanRenderer.endSwapChainRenderPass(commandBuffer);
			this->vulkanRenderer.endFrame();
		}

		this->inputHandler.reset();
		timer.stop();
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

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::toggleFullscreen( void )
{
	this->vulkanWindow.toggleFullscreen();
	this->vulkanRenderer.recreateSwapChain();
	this->camera.updateAspect(this->vulkanWindow.getAspectRatio());

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
