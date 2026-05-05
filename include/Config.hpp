#pragma once

#include "Vulkan.hpp"


namespace vox {

struct Config
{
	static constexpr bool fullScreenMode = true;
	static constexpr ui32 defaultWindowWidth = 1300;
	static constexpr ui32 defaultWindowHeight = 1300;
	static constexpr ui32 minimumViewingDistance = 160;

	static constexpr ui32 worldSeed = 0U;

	static constexpr vec3 cameraStartPos{165.0f, 225.0f, 165.0f};
	static constexpr vec3 cameraForward{20.0f, 0.0f, 20.0f};		// camera has weird pitch rotations if y is not 0

	static constexpr vec3 lightDirection{0.0f, -300.0f, 0.0f};
	static constexpr vec3 lightAmbientColor{0.2f, 0.2f, 0.2f};
	static constexpr vec3 lightColor{0.6f, 0.6f, 0.6f};
	static constexpr vec3 lightSpecularColor{0.1f, 0.1f, 0.1f};

	static constexpr i32 chunkLength = 16U;
	static constexpr i32 chunkHeight = 256U;

	static constexpr float movementSpeed = 100.0f;
	static constexpr float lookSpeed = 25.0f;

	static constexpr char skyboxVertShaderPath[] = "build/skybox.vert.spv";
	static constexpr char skyboxFragShaderPath[] = "build/skybox.frag.spv";
	static constexpr char simpleVertShaderPath[] = "build/terrain.vert.spv";
	static constexpr char simpleFragShaderPath[] = "build/terrain.frag.spv";

	static constexpr char textureStonePath[] = "textures/texture_stone_mono.jpeg";
	static constexpr char textureWaterPath[] = "textures/texture_water_mono.jpeg";
	static constexpr char textureDirtPath[] = "textures/texture_dirt_atlas.jpeg";
	static constexpr char textureSkyboxPath[] = "textures/skybox1.png";
};

} // namespace vox
