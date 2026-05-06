#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

#include "TypeAliases.hpp"


namespace vox {

class KeyboardInput
{
	public:

	KeyboardInput() noexcept;
	~KeyboardInput() noexcept = default;
	KeyboardInput(const KeyboardInput& other) = delete;
	KeyboardInput(KeyboardInput&& other) = delete;
	KeyboardInput& operator=(const KeyboardInput& other) = delete;
	KeyboardInput& operator=(KeyboardInput&& other) = delete;

	void	reset() noexcept;

	static constexpr i32 maxKeys = GLFW_KEY_LAST + 1;

	std::array<bool, maxKeys>	keyStates;
	std::array<bool, maxKeys>	keysPressed;
	std::array<bool, maxKeys>	keysReleased;
	std::array<bool, maxKeys>	keysRepeated;
};

}	// namespace vox
