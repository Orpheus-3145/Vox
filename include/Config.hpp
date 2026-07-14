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

	static constexpr ui32 worldSeed = 0U;

	static constexpr vec3 cameraStartPos{0.0f, 225.0f, 0.0f};
	static constexpr vec3 cameraForward{20.0f, 0.0f, 20.0f};		// camera has weird pitch rotations if y is not 0

	static constexpr vec3 lightDirection{0.0f, -300.0f, 0.0f};
	static constexpr vec3 lightAmbientColor{0.2f, 0.2f, 0.2f};
	static constexpr vec3 lightColor{0.6f, 0.6f, 0.6f};
	static constexpr vec3 lightSpecularColor{0.1f, 0.1f, 0.1f};

	static constexpr vec3 startingPosition{0.0f, 225.0f, 0.0f};

	static constexpr vec4 backgroundColor{0.0f, 0.0f, 0.0f, 1.0f};
	static constexpr vec4 fontColor{0.0f, 1.0f, 0.0f, 1.0f};

	static constexpr i32 chunkLength = 16U;
	static constexpr i32 chunkHeight = 256U;
	static constexpr i32 seaLevel = 64U;

	static constexpr float movementSpeed = 20.0f;
	static constexpr float lookSpeed = 75.0f;

	static constexpr char skyboxVertShaderPath[] = "build/skybox.vert.spv";
	static constexpr char skyboxFragShaderPath[] = "build/skybox.frag.spv";
	static constexpr char terrainVertShaderPath[] = "build/terrain.vert.spv";
	static constexpr char terrainFragShaderPath[] = "build/terrain.frag.spv";
	static constexpr char terrainNoLightVertShaderPath[] = "build/terrainNoLight.vert.spv";
	static constexpr char terrainNoLightFragShaderPath[] = "build/terrainNoLight.frag.spv";
	static constexpr char textVertShaderPath[] = "build/text.vert.spv";
	static constexpr char textFragShaderPath[] = "build/text.frag.spv";

	static constexpr char textureStone1Path[] = "textures/texture_stone_mono_1.jpeg";
	static constexpr char textureStone2Path[] = "textures/texture_stone_mono_2.jpeg";
	static constexpr char textureDirt1Path[] = "textures/texture_dirt_atlas.jpeg";
	static constexpr char textureDirt2Path[] = "textures/texture_dirt_mono.jpeg";
	static constexpr char textureWaterPath[] = "textures/texture_water_mono.jpeg";
	static constexpr char textureSkyboxPath[] = "textures/skybox1.png";
	static constexpr char fontPath[] = "font/RobotoMono-Regular.ttf";

	static constexpr ve::MaterialData dirtMaterial{
		vec4(0.15f, 0.15f, 0.15f, 1.0f),
		vec4(0.2f, 0.2f, 0.2f, 1.0f),
		vec4(0.2f, 0.2f, 0.2f, 1.0f),
		8.0f,
		1.0f
	};

	static constexpr ve::MaterialData stoneMaterial{
		vec4(0.25f, 0.22f, 0.20f, 1.0f),
		vec4(0.55f, 0.50f, 0.46f, 1.0f),
		vec4(0.10f, 0.10f, 0.10f, 1.0f),
		8.0f,
		1.0f
	};

	static constexpr ve::LightData lightMaterial{
		vec4{0.2f, 0.2f, 0.2f, 1.0f},
		vec4{0.6f, 0.6f, 0.6f, 1.0f},
		vec4{0.1f, 0.1f, 0.1f, 1.0f},
		vec4{0.0f, -300.0f, 0.0f, 0.0f}
	};
};

} // namespace vox
