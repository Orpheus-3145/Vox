#include "InputHandler.hpp"


namespace vox {

void	InputHandler::setCallbacks(GLFWwindow* window)
{
	glfwSetWindowUserPointer(window, this);
	// keyboard key pression callback
	glfwSetKeyCallback(window, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
	{
		(void)scancode;
		(void)mods;
		InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		switch(action)
		{
			case GLFW_PRESS:
				handler->keyboard.keysPressed[key] = true;
				break;
			case GLFW_RELEASE:
				handler->keyboard.keysReleased[key] = true;
				handler->keyboard.keysPressed[key] = false;
				break;
			case GLFW_REPEAT:
				handler->keyboard.keysRepeated[key] = true;
				break;
			default:
				break;
		}

		if (handler->isKeyPressed(GLFW_KEY_T))
		{
			handler->toggleFpsMode(window);
		}
		else if (handler->isKeyPressed(GLFW_KEY_ESCAPE))
		{
			handler->closeWindow(window);
		}
		else if (handler->isKeyPressed(GLFW_KEY_LEFT_CONTROL) and handler->isKeyPressed(GLFW_KEY_R))
		{
			handler->fullscreenCallback();
		}
	});
	// mouse key pression callback
	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, i32 button, i32 action, i32 mods)
	{
		(void)mods;
		InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		switch(action)
		{
			case GLFW_PRESS: handler->mouse.buttonsPressed[button] = true; break;
			case GLFW_RELEASE: handler->mouse.buttonsReleased[button] = true; break;
			default: break;
		}
	});
	// mouse scroll callback
	glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset)
	{
		(void)window;
		(void)xoffset;
		(void)yoffset;
	});
	// change cursor position callback
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double posX, double posY) {
		if (glfwGetWindowAttrib(window, GLFW_FOCUSED) == false)
			return;
		InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

		vec2 cursorPos{static_cast<float>(posX), static_cast<float>(posY)};
		if (handler->fpsMode == true)
			handler->mouseCallback(cursorPos);
		handler->setCursorPos(cursorPos);
	});
	// window resize callback
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, i32 w, i32 h) {
		InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
		handler->resizeCallback(w, h);
	});
}

void InputHandler::reset() noexcept
{
	keyboard.reset();
	mouse.reset();
}

void InputHandler::toggleFpsMode( GLFWwindow* window ) noexcept
{
	this->fpsMode = !this->fpsMode; 
	glfwSetInputMode(window, GLFW_CURSOR, this->fpsMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void InputHandler::closeWindow( GLFWwindow* window ) const noexcept
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

}	// namespace vox
