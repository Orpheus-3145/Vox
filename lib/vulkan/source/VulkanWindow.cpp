#include "VulkanWindow.hpp"
#include <iostream>
#include <cassert>


namespace ve {

VulkanWindow::VulkanWindow(const char* title, bool fullScreen, int32_t width, int32_t height) :
	widthNotFullscreen(width),
	heightNotFullscreen(height)
{
	assert( width > 0 && height > 0 && "Invalid window size provided");

	if (glfwInit() == GLFW_FALSE)
	{
		throw std::runtime_error("failed to start GLFW");
	}

	monitor = glfwGetPrimaryMonitor();
	if (monitor == nullptr)
	{
		throw std::runtime_error("error while fetching primary monitor");
	}

	monitorInfo = glfwGetVideoMode(monitor);
	if (monitorInfo == nullptr)
	{
		throw std::runtime_error("error occurred while fetching monitor configuration");
	}
	xPosNotFullscreen = (monitorInfo->width - widthNotFullscreen) / 2;
	yPosNotFullscreen = (monitorInfo->height - heightNotFullscreen) / 2;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	if (fullScreen == true)
	{
		glfwWindowHint(GLFW_RED_BITS, monitorInfo->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, monitorInfo->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, monitorInfo->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, monitorInfo->refreshRate);
	}

	window = glfwCreateWindow(widthNotFullscreen, heightNotFullscreen, title, nullptr, nullptr);
	if (window == nullptr)
	{
		throw std::runtime_error("failed to create GLFW window");
	}

	if (fullScreen == true)
	{
		glfwSetWindowMonitor(window, monitor, 0, 0, monitorInfo->width, monitorInfo->height, monitorInfo->refreshRate);
	}
	else
	{
		glfwSetWindowPos(window, xPosNotFullscreen, yPosNotFullscreen);
	}
}

VulkanWindow::VulkanWindow(VulkanWindow&& other) :
	monitor{other.monitor},
	monitorInfo{other.monitorInfo},
	window{other.window}
{
	other.window = nullptr;
	other.monitor = nullptr;
}

VulkanWindow::~VulkanWindow()
{
	if (window)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}

VkExtent2D	VulkanWindow::getWindowSize() const noexcept
{
	int32_t w, h;
	glfwGetWindowSize(this->window, &w, &h);

	return VkExtent2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}

void	VulkanWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) const
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface");
	}
}

void	VulkanWindow::resetWindowSize(int32_t width, int32_t height) noexcept
{
	if (isFullscreenWindow() == false)
	{
		widthNotFullscreen = width;
		heightNotFullscreen = height;
		glfwGetWindowPos(window, &xPosNotFullscreen, &yPosNotFullscreen);
	}
}

void	VulkanWindow::toggleFullscreen() noexcept
{
	if (isFullscreenWindow())
	{
		glfwSetWindowMonitor(window, nullptr, xPosNotFullscreen, yPosNotFullscreen, widthNotFullscreen, heightNotFullscreen, GLFW_DONT_CARE);
	}
	else
	{
		glfwGetWindowPos(window, &xPosNotFullscreen, &yPosNotFullscreen);
		glfwSetWindowMonitor(window, monitor, 0, 0, monitorInfo->width, monitorInfo->height, monitorInfo->refreshRate);
	}
}

}	// namespace ve
