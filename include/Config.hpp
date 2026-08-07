#pragma once

#include "Vulkan.hpp"


namespace vox {

struct Config
{
	static constexpr bool lightingMode = true;
	static constexpr bool fullScreenMode = true;

	static constexpr ui32 defaultWindowWidth = 1920;
	static constexpr ui32 defaultWindowHeight = 1080;
	static constexpr ui32 minimumViewingDistance = 160;

	static constexpr size_t	maxVRAM = 128 * 1024 * 1024; 	// 128 MiB
	static constexpr ui32 worldSeed = 314159263U;
	static constexpr ui32 worldLength = 32U;
	static constexpr ui32 worldHeight = 256U;

	static constexpr vec3 cameraStartPos{worldLength / 2.0f, 200.0f, worldLength / 2.0f};
	static constexpr vec3 cameraForward{0.0f, 0.0f, -1.0f};		// camera has weird pitch rotations if y is not 0

	static constexpr vec3 lightDirection{0.0f, -300.0f, 0.0f};
	static constexpr vec3 lightAmbientColor{0.2f, 0.2f, 0.2f};
	static constexpr vec3 lightColor{0.6f, 0.6f, 0.6f};
	static constexpr vec3 lightSpecularColor{0.1f, 0.1f, 0.1f};

	static constexpr vec3 startingPosition{0.0f, 1.0f, 0.0f};

	static constexpr vec4 backgroundColor{0.0f, 0.0f, 0.0f, 1.0f};
	static constexpr vec4 fontColor{0.0f, 1.0f, 0.0f, 1.0f};

	static constexpr i32 chunkLength = 16U;		// NB remove
	static constexpr i32 chunkHeight = 256U;	// NB remove

	static constexpr float normalSpeed = 20.0f;
	static constexpr float fastSpeed = 100.0f;
	static constexpr float lookSpeed = 75.0f;

	static constexpr char skyboxVertShaderPath[] = "build/shaders/skybox.vert.spv";
	static constexpr char skyboxFragShaderPath[] = "build/shaders/skybox.frag.spv";
	static constexpr char terrainVertShaderPath[] = "build/shaders/terrain.vert.spv";
	static constexpr char terrainFragShaderPath[] = "build/shaders/terrain.frag.spv";
	static constexpr char terrainNoLightVertShaderPath[] = "build/shaders/terrainNoLight.vert.spv";
	static constexpr char terrainNoLightFragShaderPath[] = "build/shaders/terrainNoLight.frag.spv";
	static constexpr char textVertShaderPath[] = "build/shaders/text.vert.spv";
	static constexpr char textFragShaderPath[] = "build/shaders/text.frag.spv";

	static constexpr char textureStone1Path[] = "textures/texture_stone_mono_1.jpeg";
	static constexpr char textureStone2Path[] = "textures/texture_stone_mono_2.jpeg";
	static constexpr char textureDirt1Path[] = "textures/texture_dirt_atlas.jpeg";
	static constexpr char textureDirt2Path[] = "textures/texture_dirt_mono.jpeg";
	static constexpr char textureSkyboxPath[] = "textures/skybox1.png";
	static constexpr char fontPath[] = "font/RobotoMono-Regular.ttf";
};

} // namespace vox
