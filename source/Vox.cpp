#include "Vox.hpp"
#include "Stopwatch.hpp"
#include "Utils.hpp"
#include "World.hpp"

#include <iostream>
#include <cassert>
#include <future>

namespace vox {

// Phong lighting model, a combination of the following:
// Ambient lighting: even when it is dark there is usually still some light somewhere in the world
// 		(the moon, a distant light) so objects are almost never completely dark. To simulate this
// 		we use an ambient lighting constant that always gives the object some color.
// Diffuse lighting: simulates the directional impact a light object has on an object. This is the
// 		most visually significant component of the lighting model. The more a part of an object
// 		faces the light source, the brighter it becomes.
// Specular lighting: simulates the bright spot of a light that appears on shiny objects. Specular
// 		highlights are more inclined to the color of the light than the color of the object.
constexpr ve::MeshMaterial dirtMaterial{
	vec4(0.15f),						// ambientColor
	vec4(0.2f, 0.2f, 0.2f, 1.0f),		// diffuseColor
	vec4(0.2f),							// specularColor
	8.0f,								// shininess
	1.0f,								// opacity
	1,									// refractionIndex
	2									// illuminationModel
};


void LightUBO::updateLightDir( vec3 const& lightDir, mat4 const& viewMatrix ) noexcept
{
	// light dir goes from fragment to light source, but shaders assume the opposite, so it has to be negated
	// also, viewMatrix has to be in row major
	vec4 lightDir4 = vec4{lightDir * -1, 0.0f};
	lightDir4 = viewMatrix * lightDir4;
	lightDir4.normalize();
	this->lightDir = lightDir4;
}

/**
 * Create the engine of the game
 */
Vox::Vox( void ) :
	vulkanWindow{"ft_vox", Config::fullScreenMode, Config::defaultWindowWidth, Config::defaultWindowHeight},
	vulkanDevice{vulkanWindow},
	vulkanRenderer{vulkanWindow, vulkanDevice},
	vulkanSetFactory{vulkanDevice},
	camera{Config::cameraStartPos, Config::cameraForward, this->vulkanWindow.getAspectRatio()},
	voxelMap{threadManager},
	inputHandler{
		[this](vec2 const& cursorPos) { this->rotateCameraFromCursorPos(cursorPos); },
		[this](i32 width, i32 height) { this->resizeWindow(width, height); },
		[this](void) { this->toggleFullscreen(); }
	}
{
	this->voxelMap.init();
	this->inputHandler.setCallbacks(this->vulkanWindow.getGLFWwindow());
}

void Vox::setupVulkan( void )
{
	ui32	maxSetsToCreate = 5;
	ui32	nUniformDescriptors = 2;
	ui32	nSamplerDescriptors = 4;

	this->terrainObject = std::make_unique<ve::VulkanObject>();
	this->undergroundObject = std::make_unique<ve::VulkanObject>();
	this->skyboxObject = std::make_unique<ve::VulkanObject>();

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
	this->uboDescriptorSet->addBufferDescriptor(0, sizeof(ViewProjectUBO));
	this->uboDescriptorSet->addBufferDescriptor(1, sizeof(LightUBO));

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

	this->terrainObject->setModel(this->voxelMap.createNewModelTerrain(vulkanDevice));
	this->undergroundObject->setModel(this->voxelMap.createNewModelUnderground(vulkanDevice));
	this->skyboxObject->setModel(this->createVoxelMesh());

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		this->uboDescriptorSet->getDescriptorSetLayout(),
		this->textTerrainDescriptorSet->getDescriptorSetLayout(),
		this->textUndergroundDescriptorSet->getDescriptorSetLayout(),
		this->textSkyboxDescriptorSet->getDescriptorSetLayout()
	};
	this->terrainPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		descriptorSetLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		Config::simpleVertShaderPath,
		Config::simpleFragShaderPath,
		this->terrainObject->getVboLayout(),
		false,
		sizeof(MeshData)
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
}

/**
 * Run the rendering loop
 */
void Vox::run( void )
{
	ViewProjectUBO matrixUbo(
		this->camera.getViewMatrix(),
		this->camera.getProjectionMatrix()
	);
	LightUBO lightUbo(
		Config::lightDirection,
		this->camera.getViewMatrix(false),
		Config::lightAmbientColor,
		Config::lightColor,
		Config::lightSpecularColor
	);
	MeshData terrainData(
		this->terrainObject->getModelMatrix(),
		this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()),
		dirtMaterial
	);
	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;

	float deltaTime = 0.0f;
	Stopwatch timer;
	std::future<bool>	mapUpdateResult;

	std::cout << "\n\n\n\n";
	while (vulkanWindow.shouldClose() == false)
	{
		timer.start();
		glfwPollEvents();

		deltaTime = timer.elapsed(Unit::Seconds);
		this->moveCamera(deltaTime);

		vec3 playerPos = this->camera.getCameraPos();
		this->inputHandler.reset();

		if (voxelMap.update(playerPos) == true)
		{
			this->terrainObject->setModel(this->voxelMap.createNewModelTerrain(vulkanDevice));
			this->undergroundObject->setModel(this->voxelMap.createNewModelUnderground(vulkanDevice)); // main thread
		}
		// if (mapUpdateResult.valid() == false)
		// {
		// 	mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
		// 		return voxelMap.update(playerPos);
		// 	});
		// }
		// else
		// {
		// 	const std::future_status status = mapUpdateResult.wait_for(std::chrono::milliseconds(0));

		// 	if (status == std::future_status::ready)
		// 	{
		// 		const bool changed = mapUpdateResult.get(); // consumes future; now invalid

		// 		if (changed == true)
		// 		{
		// 			this->terrainObject->setModel(this->voxelMap.createNewModelTerrain(vulkanDevice));
		// 			this->undergroundObject->setModel(this->voxelMap.createNewModelUnderground(vulkanDevice)); // main thread
		// 		}
		// 		mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
		// 			return voxelMap.update(playerPos);
		// 		});
		// 	}
		// }

		VkCommandBuffer commandBuffer = this->vulkanRenderer.beginFrame();
		if (commandBuffer != nullptr)
		{
			this->vulkanRenderer.beginSwapChainRenderPass(commandBuffer);

			ui32 currentFrame = this->vulkanRenderer.getCurrentFrameIndex();
			this->uboDescriptorSet->setCurrentFrame(currentFrame);
			this->textTerrainDescriptorSet->setCurrentFrame(currentFrame);
			this->textUndergroundDescriptorSet->setCurrentFrame(currentFrame);
			this->textSkyboxDescriptorSet->setCurrentFrame(currentFrame);

			if (this->countFramesToUpdate > 0)
			{
				matrixUbo.updateView(this->camera.getViewMatrix());
				matrixUbo.updateProjection(this->camera.getProjectionMatrix());
				this->uboDescriptorSet->updateUbo(0, matrixUbo.getData());
				lightUbo.updateLightDir(Config::lightDirection, this->camera.getViewMatrix(false));
				this->uboDescriptorSet->updateUbo(1, lightUbo.getData());
				this->countFramesToUpdate--;
			}

			this->terrainPipeline->bindPipeline(commandBuffer);
			this->uboDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 0U);
			this->textTerrainDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 1U);
			this->terrainPipeline->updatePushConstants(commandBuffer, terrainData.getData());

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
	float	moveScalar = std::min(deltaTime * Config::movementSpeed, static_cast<float>(Config::minimumViewingDistance));
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
		this->camera.move(moveDirection);
		this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
		// vec3 relativeMoveDirection = this->camera.getRelativeMoveDirection(moveDirection);

		// this->voxelMap.detectCollision(relativeMoveDirection);
		// this->camera.move(relativeMoveDirection);
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
