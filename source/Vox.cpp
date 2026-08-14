#include <iostream>
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
	navigator{vulkanDevice, Config::worldLength, Config::worldHeight, Config::maxVRAM, Config::worldSeed},
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
	this->backgroundUIObject = std::make_unique<ve::VulkanObject>();
	this->textUIObject = std::make_unique<ve::VulkanObject>();

	this->setupVulkanBuffers();
	this->setupVulkanDescSets();
	this->setupVulkanPipelines();

	this->countFramesToUpdate = ve::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Vox::run( void )
{
	Stopwatch			fpsTimer, printTimer;
	std::future<void>	mapUpdateResult;
	ui32				currentFrame = 0U, fps = 0U;
	VkCommandBuffer		commandBuffer = VK_NULL_HANDLE;

	// NB add voxel destruction
	this->skyboxObject->setModel(this->createModel(voxelAtlasVertexes(vec3(-0.5f)), voxelIndexes(), 0U, ve::ONLY_VERTEX_LAYOUT));
	this->updateMap(mapUpdateResult);
	while (this->navigator.isReady() == false);

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
				fps = static_cast<i32>(1.0f / fpsTimer.elapsed(Unit::Seconds));
				printTimer.reset();
			}
			this->drawUI(commandBuffer, currentFrame, fps);

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

	std::vector<std::string>		texturePaths{
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
	this->UIDescriptorSet = this->vulkanSetFactory.createDescriptorSet(fontSetBindings);
	this->UIDescriptorSet->updateDescriptor(0U, this->textDataUbo->getData());
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
	setLayouts[1] = this->UIDescriptorSet->getLayout();
	this->UIPipeline = ve::VulkanPipeline::createPipeline(
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

std::shared_ptr<ve::VulkanModel> Vox::createModel( ve::VertexVector const& vertexes, ve::IndexVector const& indexes, ui32 binding, ve::VertexLayout layout )
{
	return std::make_shared<ve::VulkanModel>(this->vulkanDevice, vertexes, indexes, binding, layout);
}

void Vox::moveCamera( float deltaTime )
{
	float	movementSpeed = (this->walkFast) ? Config::fastSpeed : Config::normalSpeed;
	movementSpeed /= VOXEL_SIZE;

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

void Vox::updateMap( std::future<void>& mapUpdateResult )
{
	vec3 playerPos = this->camera.getCameraPos();
	
	if (mapUpdateResult.valid() == false)
	{
		mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
			this->navigator.spawnCloseByWorldsMT(playerPos);
		});
	}
	else
	{
		std::future_status status = mapUpdateResult.wait_for(std::chrono::milliseconds(0));
	
		if (status == std::future_status::ready)
		{
			mapUpdateResult = std::async(std::launch::async, [this, playerPos] {
				this->navigator.spawnCloseByWorldsMT(playerPos);
			});
		}
	}
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

	this->navigator.drawTerrain(commandBuffer, this->camera.getFrustum());

	indexes.indexModel = 1U;
	indexes.indexMaterial = 1U;
	indexes.indexTexture = 1U;
	this->terrainPipeline->updatePushConstants(commandBuffer, &indexes);

	this->navigator.drawCaves(commandBuffer, this->camera.getFrustum());
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

void Vox::drawUI(VkCommandBuffer commandBuffer, ui32 currentFrame, ui32 fps)
{
	IndexUniforms	indexes{};
	size_t const	sizeFont = 32 /* NB ugly */, padding = sizeFont / 2;
	ve::VulkanSamplerDescriptor const* fontTexture = this->UIDescriptorSet->getSamplerDescriptor(1U);

	std::string	text = "FPS: " + std::to_string(fps);
	vec2i 		textPosition{static_cast<i32>(this->vulkanWindow.getWindowSize().width), padding / 2};
	ve::UIvertexes fpsData = fontTexture->getUIvertexes(text, textPosition, 0U, true);

	text = "GPU memory used: " + formatBytes(this->navigator.getMemoryUsed());
	textPosition.y += sizeFont + padding;
	ve::UIvertexes memoryData = fontTexture->getUIvertexes(text, textPosition, 0U, true);

	text = "Current pos: " + this->camera.formatCameraPos();
	textPosition.y += sizeFont + padding;
	ve::UIvertexes posData = fontTexture->getUIvertexes(text, textPosition, 0U, true);

	this->UIPipeline->bindPipeline(commandBuffer);
	this->uboDescriptorSet[currentFrame]->bindSet(commandBuffer, *this->UIPipeline, 0U);
	this->UIDescriptorSet->bindSet(commandBuffer, *this->UIPipeline, 1U);

	ve::VertexVector allBgVertex;
	allBgVertex.insert(allBgVertex.end(), fpsData.bgVertexes.begin(), fpsData.bgVertexes.end());
	allBgVertex.insert(allBgVertex.end(), memoryData.bgVertexes.begin(), memoryData.bgVertexes.end());
	allBgVertex.insert(allBgVertex.end(), posData.bgVertexes.begin(), posData.bgVertexes.end());
	this->backgroundUIObject->setModel(this->createModel(allBgVertex, ve::IndexVector(), 0U, ve::FONT_MODEL_LAYOUT));

	indexes.indexFontColor = 0;		// background color index
	this->UIPipeline->updatePushConstants(commandBuffer, &indexes);

	this->backgroundUIObject->bindBuffer(commandBuffer);
	this->backgroundUIObject->draw(commandBuffer);

	ve::VertexVector allTextVertex;
	allTextVertex.insert(allTextVertex.end(), fpsData.textVertexes.begin(), fpsData.textVertexes.end());
	allTextVertex.insert(allTextVertex.end(), memoryData.textVertexes.begin(), memoryData.textVertexes.end());
	allTextVertex.insert(allTextVertex.end(), posData.textVertexes.begin(), posData.textVertexes.end());
	this->textUIObject->setModel(this->createModel(allTextVertex, ve::IndexVector(), 0U, ve::FONT_MODEL_LAYOUT));

	indexes.indexFontColor = 1;		// text color index
	this->UIPipeline->updatePushConstants(commandBuffer, &indexes);

	this->textUIObject->bindBuffer(commandBuffer);
	this->textUIObject->draw(commandBuffer);
}

}	// namespace vox
