#pragma once

#include "KeyboardInput.hpp"
#include "MouseInput.hpp"
#include "TypeAliases.hpp"

#include <functional>


namespace vox {

class InputHandler
{
	public:

	InputHandler() = default;
	InputHandler(std::function<void(vec2 const&)> mouseCb, std::function<void(i32, i32)> resizeCb, std::function<void()> fullscreenCallback) noexcept :
		mouseCallback(mouseCb),
		resizeCallback(resizeCb),
		fullscreenCallback(fullscreenCallback) {};
	~InputHandler() noexcept = default;
	InputHandler(const InputHandler&) = delete;
	InputHandler& operator=(const InputHandler&) = delete;

	void	setCallbacks(GLFWwindow* window);
	void	reset() noexcept;

	bool	isKeyPressed(i32 key) const { return keyboard.keysPressed[key]; }
	bool	isKeyReleased(i32 key) const { return keyboard.keysReleased[key]; }
	bool	isKeyRepeated(i32 key) const { return keyboard.keysRepeated[key]; }
	bool	isMouseButtonPressed(i32 button) const { return mouse.buttonsPressed[button]; }
	bool	isMouseButtonReleased(i32 button) const { return mouse.buttonsReleased[button]; }

	void		setCursorPos(vec2 const& newPos) noexcept { this->mouse.setCursorPos(newPos); };
	vec2 const&	getCursorPos() const noexcept { return this->mouse.getCursorPos(); };

	void	toggleFpsMode(GLFWwindow* window) noexcept;
	void	closeWindow(GLFWwindow* window) const noexcept;

	private:

	KeyboardInput	keyboard;
	MouseInput		mouse;
	bool			fpsMode{false};

	std::function<void(vec2 const&)>	mouseCallback;
	std::function<void(i32, i32)>		resizeCallback;
	std::function<void()>				fullscreenCallback;
};

} // namespace vox
