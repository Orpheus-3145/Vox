#include <iostream>
#include <cassert>
#include <future>

#include "Vox.hpp"
#include "Stopwatch.hpp"
#include "Utils.hpp"


namespace vox {

Vox::Vox( void ) :
	vulkanWindow{"vox", Config::fullScreenMode, Config::defaultWindowWidth, Config::defaultWindowHeight},
	vulkanDevice{vulkanWindow},
	vulkanRenderer{vulkanWindow, vulkanDevice},
	vulkanSetFactory{vulkanDevice},
	camera{Config::cameraStartPos, Config::cameraForward.normalized(), this->vulkanWindow.getWindowSize()},
	navigator{Config::worldLength, Config::worldHeight, Config::maxVRAM, Config::worldSeed},
	inputHandler{
		[this](vec2 const& cursorPos) { this->rotateCameraFromCursorPos(cursorPos); },
		[this](i32 width, i32 height) { this->resizeWindow(width, height); },
		[this](void) { this->toggleFullscreen(); }
	}
{
	this->inputHandler.setCallbacks(this->vulkanWindow.getGLFWwindow());

	this->terrainObject = std::make_unique<ve::VulkanObject>();
	this->caveObject = std::make_unique<ve::VulkanObject>();
	this->skyboxObject = std::make_unique<ve::VulkanObject>();
	this->fpsBackgroundObject = std::make_unique<ve::VulkanObject>();
	this->fpsTextObject = std::make_unique<ve::VulkanObject>();
	this->memoryBackgroundObject = std::make_unique<ve::VulkanObject>();
	this->memoryTextObject = std::make_unique<ve::VulkanObject>();

	this->setupVulkanBuffers();
	this->setupVulkanDescSets();
	this->setupVulkanPipelines();

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::run( void )
{
	Stopwatch			fpsTimer, printTimer;
	std::future<bool>	mapUpdateResult;
	ui32				currentFrame = 0U;
	i32					fps = 0;
	std::string			UItext = "FPS: 0", memoryUitext = "GPU memory used: 0b";
	VkCommandBuffer		commandBuffer = VK_NULL_HANDLE;
	vec2i 				fpsCounterPosition{static_cast<i32>(this->vulkanWindow.getWindowSize().width), 0};
	vec2i 				memoryPosition{static_cast<i32>(this->vulkanWindow.getWindowSize().width), 48};

	this->skyboxObject->setModel(this->createSkyboxModel());

	printTimer.start();
	while (vulkanWindow.shouldClose() == false)
	{
		fpsTimer.start();
		glfwPollEvents();

		this->moveCamera(fpsTimer.elapsed(Unit::Seconds));
		this->updateMap(mapUpdateResult);

		commandBuffer = this->vulkanRenderer.beginFrame();
		if (commandBuffer != nullptr)
		{
			this->vulkanRenderer.beginSwapChainRenderPass(commandBuffer);
			currentFrame = this->vulkanRenderer.getCurrentFrameIndex();

			if (this->countFramesToUpdate > 0)
			{
				this->updateUniforms(currentFrame);
			}

			this->drawTerrain(commandBuffer, currentFrame);
			this->drawSkybox(commandBuffer, currentFrame);

			printTimer.stop();
			if (printTimer.elapsed(Unit::Seconds) > 0.5)
			{
				fps = static_cast<int> (1.0f / fpsTimer.elapsed(Unit::Seconds));
				UItext = "FPS: " + std::to_string(fps);
				printTimer.reset();
			}
			this->drawTextFPS(commandBuffer, currentFrame, UItext, fpsCounterPosition);

			memoryUitext = "GPU memory used: " + formatBytes(this->navigator.getMemoryUsed());
			this->drawTextMemory(commandBuffer, currentFrame, memoryUitext, memoryPosition);

			this->vulkanRenderer.endSwapChainRenderPass(commandBuffer);
			this->vulkanRenderer.endFrame();
		}

		this->inputHandler.reset();
		fpsTimer.stop();
	}
	vkDeviceWaitIdle(vulkanDevice.device());
}

void Vox::rotateCameraFromCursorPos( vec2 const& currPos )	
{
	vec2 const& oldPos = this->inputHandler.getCursorPos();

	float yaw = (currPos.x - oldPos.x) * CameraSettings::cameraSensitivity;
	float pitch = (oldPos.y - currPos.y) * CameraSettings::cameraSensitivity;  // reversed since y-coordinates range from bottom to top
	this->camera.rotate(pitch, yaw, 0.0f);

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::resizeWindow( ui32 width, ui32 height )
{
	this->vulkanWindow.resetWindowSize(static_cast<i32>(width), static_cast<i32>(height));
	this->vulkanRenderer.recreateSwapChain();

	WindowSize size = this->vulkanWindow.getWindowSize();
	this->camera.updateWindowSize(size.width, size.height);

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::toggleFullscreen( void )
{
	this->vulkanWindow.toggleFullscreen();
	this->vulkanRenderer.recreateSwapChain();

	WindowSize size = this->vulkanWindow.getWindowSize();
	this->camera.updateWindowSize(size.width, size.height);

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::setupVulkanBuffers( void )
{
	// uniform buffer for view and projection matrixes
	this->matrixUbo = std::make_unique<ViewProjectUniform>(
		this->camera.getViewMatrix(),
		this->camera.getProjectionMatrix(),
		this->camera.getOrthographicMatrix()
	);

	// uniform buffers for per-mesh data: model and normal matrixes, materials, lights
	this->materialsUbo = std::make_unique<MeshUniform>();
	this->materialsUbo->updateModelMatrix(0, this->terrainObject->getModelMatrix());
	this->materialsUbo->updateModelMatrix(1, this->caveObject->getModelMatrix());
	this->materialsUbo->updateModelMatrix(2, mat4::idMat());

	this->materialsUbo->updateNormalMatrix(0, this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateNormalMatrix(1, this->caveObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateNormalMatrix(2, mat4::idMat());

	this->materialsUbo->updateMaterial(0U, DIRT_MATERIAL);
	this->materialsUbo->updateMaterial(1U, STONE_MATERIAL);
	this->materialsUbo->updateLight(0U, DEFAULT_LIGHT, this->camera.getViewMatrix(false));

	this->textDataUbo = std::make_unique<TextUniform>();
	// color of the UI
	this->textDataUbo->updateColor(0U, vec4{0.0f, 0.0f, 0.0f, 1.0f});
	// text color
	this->textDataUbo->updateColor(1U, Config::fontColor);
}

void Vox::setupVulkanDescSets( void )
{
	// three sets (one for uniforms *[see later], one for textures, one for fonts)
	ui32	maxSetsToCreate = 1U * ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT + 2U;
	ui32	nUniformDescriptors = 2U * ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT + 2U;
	ui32	nSamplerDescriptors = 4U;	// 1 texture for dirt, 1 texture for stone, 1 for skybox, 1 for font

	this->vulkanSetFactory
		.setMaxSets(maxSetsToCreate)
		.addBufferPoolSize(nUniformDescriptors)
		.addSamplerPoolSize(nSamplerDescriptors)
		.createPool();

	ve::VulkanBindingSet uboSetBindings;
	// UBO with matrixes equal for every mesh: view and projections
	uboSetBindings.addBufferBinding(0, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ViewProjectUniform));
	// UBO with data mesh-specific data: model, normal, lights, ...
	uboSetBindings.addBufferBinding(1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MeshUniform));

	// data inside this set changes per frame so a copy of such data is needed for every frame buffer
	// to avoid modifyind something which is used by another frame buffer
	this->uboDescriptorSet.resize(ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0U; i < ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		this->uboDescriptorSet[i] = this->vulkanSetFactory.createDescriptorSet(uboSetBindings);
	}

	std::vector<std::string>	texturePaths{
		Config::textureDirt1Path,
		Config::textureStone1Path,
	};
	std::vector<ve::TextureType>	textureTypes(2, ve::TextureType::TEXTURE_PLAIN);

	ve::VulkanBindingSet textureSetBindings;
	// normal textures
	textureSetBindings.addSamplerArrayBinding(0U, VK_SHADER_STAGE_FRAGMENT_BIT, texturePaths, textureTypes);
	// cubemap texture
	textureSetBindings.addSamplerBinding(1U, VK_SHADER_STAGE_FRAGMENT_BIT, Config::textureSkyboxPath, ve::TextureType::TEXTURE_CUBEMAP);
	this->textureDescriptorSet = this->vulkanSetFactory.createDescriptorSet(textureSetBindings);

	ve::VulkanBindingSet fontSetBindings;
	// array of uniforms containing colors for the font (text, background, ...)
	fontSetBindings.addBufferBinding(0U, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TextUniform));
	// texture/sampler of the font used
	fontSetBindings.addSamplerBinding(1U, VK_SHADER_STAGE_FRAGMENT_BIT, Config::fontPath, ve::TextureType::TEXTURE_FONT);
	this->fontDescriptorSet = this->vulkanSetFactory.createDescriptorSet(fontSetBindings);
	this->fontDescriptorSet->updateDescriptor(0U, this->textDataUbo->getData());
}

void Vox::setupVulkanPipelines( void )
{
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

	std::vector<VkDescriptorSetLayout> setLayouts;
	setLayouts.push_back(this->uboDescriptorSet[0]->getLayout());
	setLayouts.push_back(this->textureDescriptorSet->getLayout());

	// main pipeline to render the map (surface terrain + underground)
	this->terrainPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		setLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		vertexShader,
		fragmentShader,
		ve::VulkanModel::getModelLayout(0U),
		ve::TEXTURE_PLAIN,
		sizeof(IndexUniforms)
	);

	// skybox rendering
	this->skyboxPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		setLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		Config::skyboxVertShaderPath,
		Config::skyboxFragShaderPath,
		ve::VulkanModel::getModelLayout(0U, ve::ONLY_VERTEX_LAYOUT),
		ve::TEXTURE_CUBEMAP,
		sizeof(IndexUniforms)
	);

	// text/UI rendering
	setLayouts[1] = this->fontDescriptorSet->getLayout();
	this->fpsCounterPipeline = ve::VulkanPipeline::createPipeline(
		this->vulkanDevice,
		setLayouts,
		this->vulkanRenderer.getSwapChainRenderPass(),
		Config::textVertShaderPath,
		Config::textFragShaderPath,
		ve::VulkanModel::getModelLayout(0U, ve::FONT_MODEL_LAYOUT),
		ve::TEXTURE_FONT,
		sizeof(IndexUniforms)
	);
}

std::unique_ptr<ve::VulkanModel> Vox::createSkyboxModel( void ) 
{
	return std::make_unique<ve::VulkanModel>(this->vulkanDevice, voxelAtlasVertexes(vec3(-0.5f)), voxelIndexes(), 0U, ve::ONLY_VERTEX_LAYOUT);
}

void Vox::moveCamera( float deltaTime )
{
	float	movementSpeed = (this->walkFast) ? Config::fastSpeed : Config::normalSpeed;
	vec3	moveDirection = vec3::zero();
	vec3	rotation = vec3::zero();
	float	moveScalar = deltaTime * movementSpeed;
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
	if (this->inputHandler.isKeyReleased(GLFW_KEY_V)) { this->walkFast = !this->walkFast; }

	if (rotation != vec3::zero())
	{
		this->camera.rotate(rotation.x, rotation.y, 0.0f);
		this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
	}
	if (moveDirection != vec3::zero())
	{
		vec3 movement = this->camera.getRelativeMoveDirection(moveDirection);
		movement = this->navigator.checkClipping(camera.getCameraPos(), movement);
		this->camera.move(movement);
		this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
	}
}

void Vox::updateMap( std::future<bool>& mapUpdateResult )
{
	(void) mapUpdateResult;
	if (this->navigator.borderCrossed(this->camera.getCameraPos()) == true)
	{
		this->navigator.spawnCloseByWorlds(this->camera.getCameraPos());
		if (this->navigator.spawnNewModel() == true)
		{
			this->terrainObject->setModel(this->navigator.createTerrainModel(this->vulkanDevice));
			this->caveObject->setModel(this->navigator.createCaveModel(this->vulkanDevice));
		}
	}

	(void) mapUpdateResult;
	// NB add multi-threading support
	// vec3	playerPos = this->camera.getCameraPos();
	//
	// if (mapUpdateResult.valid() == false)
	// {
	// 	mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
	// 		return voxelMap.update(playerPos);
	// 	});
	// }
	// else
	// {
	// 	const std::future_status status = mapUpdateResult.wait_for(std::chrono::milliseconds(0));
	//
	// 	if (status == std::future_status::ready)
	// 	{
	// 		const bool changed = mapUpdateResult.get(); // consumes future; now invalid
	//
	// 		if (changed == true)
	// 		{
	// 			this->terrainObject->setModel(this->voxelMap.createNewTerrainModel(vulkanDevice));
	// 			this->caveObject->setModel(this->voxelMap.createNewUndergroundModel(vulkanDevice));
	// 		}
	// 		mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
	// 			return voxelMap.update(playerPos);
	// 		});
	// 	}
	// }
}

void Vox::updateUniforms(ui32 currentFrame)
{
	this->matrixUbo->updateView(this->camera.getViewMatrix());
	this->matrixUbo->updateProjection(this->camera.getProjectionMatrix());
	this->matrixUbo->updateOrthographic(this->camera.getOrthographicMatrix());
	this->uboDescriptorSet[currentFrame]->updateDescriptor(0U, this->matrixUbo->getData());

	this->materialsUbo->updateNormalMatrix(0U, this->terrainObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateNormalMatrix(1U, this->caveObject->getNormalViewMatrix(this->camera.getViewMatrixNoTranslation()));
	this->materialsUbo->updateLightDir(0U, Config::lightDirection, this->camera.getViewMatrix(false));
	this->uboDescriptorSet[currentFrame]->updateDescriptor(1U, this->materialsUbo->getData());

	this->countFramesToUpdate--;
}

void Vox::drawTerrain(VkCommandBuffer commandBuffer, ui32 currentFrame)
{
	IndexUniforms indexes{};

	this->terrainPipeline->bindPipeline(commandBuffer);

	this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->terrainPipeline, 0U);
	this->textureDescriptorSet->bindSet(commandBuffer, *this->terrainPipeline, 1U);

	indexes.indexModel = 0U;
	indexes.indexMaterial = 0U;
	indexes.indexTexture = 0U;
	this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

	this->terrainObject->bindBuffer(commandBuffer);
	this->terrainObject->draw(commandBuffer);

	indexes.indexModel = 1U;
	indexes.indexMaterial = 1U;
	indexes.indexTexture = 1U;
	this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

	this->caveObject->bindBuffer(commandBuffer);
	this->caveObject->draw(commandBuffer);
}

void Vox::drawSkybox(VkCommandBuffer commandBuffer, ui32 currentFrame)
{
	IndexUniforms indexes{};

	this->skyboxPipeline->bindPipeline(commandBuffer);

	this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->skyboxPipeline, 0U);
	this->textureDescriptorSet->bindSet(commandBuffer, *this->skyboxPipeline, 1U);

	indexes.indexModel = 2U;
	this->skyboxPipeline->updatePushConstants(commandBuffer, &indexes);

	this->skyboxObject->bindBuffer(commandBuffer);
	this->skyboxObject->draw(commandBuffer);
}

void Vox::drawTextFPS(VkCommandBuffer commandBuffer, ui32 currentFrame, std::string const& text, vec2i const& position)
{
	IndexUniforms indexes{};

	ve::VulkanSamplerDescriptor const* fontTexture = this->fontDescriptorSet->getSamplerDescriptor(1U);

	ve::FontModel fontData = fontTexture->getModelFromText(text, position, 0U, true);
	this->fpsBackgroundObject->setModel(fontData.background);
	this->fpsTextObject->setModel(fontData.text);

	this->fpsCounterPipeline->bindPipeline(commandBuffer);
	this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->fpsCounterPipeline, 0U);
	this->fontDescriptorSet->bindSet(commandBuffer, *this->fpsCounterPipeline, 1U);

	indexes.indexFontColor = 0;		// background color index
	this->fpsCounterPipeline->updatePushConstants(commandBuffer, &indexes);

	this->fpsBackgroundObject->bindBuffer(commandBuffer);
	this->fpsBackgroundObject->draw(commandBuffer);

	indexes.indexFontColor = 1;		// text color index
	this->fpsCounterPipeline->updatePushConstants(commandBuffer, &indexes);

	this->fpsTextObject->bindBuffer(commandBuffer);
	this->fpsTextObject->draw(commandBuffer);
}

void Vox::drawTextMemory(VkCommandBuffer commandBuffer, ui32 currentFrame, std::string const& text, vec2i const& position)
{
	IndexUniforms indexes{};

	ve::VulkanSamplerDescriptor const* fontTexture = this->fontDescriptorSet->getSamplerDescriptor(1U);

	ve::FontModel fontData = fontTexture->getModelFromText(text, position, 0U, true);
	this->memoryBackgroundObject->setModel(fontData.background);
	this->memoryTextObject->setModel(fontData.text);

	this->fpsCounterPipeline->bindPipeline(commandBuffer);
	this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->fpsCounterPipeline, 0U);
	this->fontDescriptorSet->bindSet(commandBuffer, *this->fpsCounterPipeline, 1U);

	indexes.indexFontColor = 0;		// background color index
	this->fpsCounterPipeline->updatePushConstants(commandBuffer, &indexes);

	this->memoryBackgroundObject->bindBuffer(commandBuffer);
	this->memoryBackgroundObject->draw(commandBuffer);

	indexes.indexFontColor = 1;		// text color index
	this->fpsCounterPipeline->updatePushConstants(commandBuffer, &indexes);

	this->memoryTextObject->bindBuffer(commandBuffer);
	this->memoryTextObject->draw(commandBuffer);
}

}	// namespace vox
