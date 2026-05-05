#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>

namespace ve {

class VulkanWindow
{
	public:

	VulkanWindow() = delete;
	VulkanWindow(const char* title, bool fullScreen = false, int32_t width = 800, int32_t height = 600);
	VulkanWindow(const VulkanWindow&) = delete;
	VulkanWindow& operator=(const VulkanWindow&) = delete;
	~VulkanWindow();

	GLFWwindow*	getGLFWwindow() const noexcept { return window; }
	float		getAspectRatio() const noexcept;
	VkExtent2D	getFramebufferExtent() const noexcept;

	bool	shouldClose() const noexcept { return glfwWindowShouldClose(window); }
	bool	wasWindowResized() const noexcept { return resized; }
	void	resetWindowResizedFlag() noexcept { resized = false; }
	
	void	createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) const;
	void	resetWindowSize(int32_t width, int32_t height) noexcept;
	void	toggleFullscreen() noexcept;
	bool	isFullscreenWindow() const noexcept { return glfwGetWindowMonitor(window) != nullptr; }

	private:

	int32_t	widthNotFullscreen;
	int32_t	heightNotFullscreen;
	int32_t	xPosNotFullscreen;
	int32_t	yPosNotFullscreen;

	bool	resized{false};

	GLFWmonitor*		monitor;
	const GLFWvidmode*	monitorInfo;
	GLFWwindow*			window;
};

}