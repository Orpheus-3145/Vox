#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "VulkanDevice.hpp"
#include "VulkanModel.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanWindow.hpp"


namespace ve {

class VulkanRenderer
{
	public:

	VulkanRenderer() = delete;
	VulkanRenderer(VulkanWindow& window, VulkanDevice& device);
	~VulkanRenderer();
	
	VulkanRenderer(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;

	VkRenderPass	getSwapChainRenderPass() const noexcept { return vulkanSwapChain->getRenderPass();}
	bool			isFrameInProgress() const noexcept { return isFrameStarted; }
	void			recreateSwapChain();

	VkCommandBuffer	getCurrentCommandBuffer() const noexcept
	{
		assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
		return commandBuffers[currentFrameIndex];
	}

	int32_t	getCurrentFrameIndex() const noexcept
	{
		assert(isFrameStarted && "Cannot get frame index when frame not in progress");
		return currentFrameIndex;
	}

	VkCommandBuffer	beginFrame();
	void			endFrame();
	void			beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void			endSwapChainRenderPass(VkCommandBuffer commandBuffer) noexcept;

	private:

	void	createCommandBuffers();

	VulkanWindow&	vulkanWindow;
	VulkanDevice&	vulkanDevice;

	std::unique_ptr<VulkanSwapChain>	vulkanSwapChain;
	std::vector<VkCommandBuffer>		commandBuffers;

	uint32_t	currentImageIndex;
	uint32_t	currentFrameIndex;
	bool		isFrameStarted;
};

} // namespace ve
