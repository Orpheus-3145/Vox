#pragma once

#include <string>
#include <memory>
#include <vector>

#include "VulkanDevice.hpp"


namespace ve {

class VulkanSwapChain
{
	public:
	
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	VulkanSwapChain(VulkanDevice& deviceRef, VkExtent2D windowExtent);
	VulkanSwapChain(VulkanDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<VulkanSwapChain> previous);
	~VulkanSwapChain();

	VulkanSwapChain() = delete;
	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

	VkFramebuffer	getFrameBuffer(int index) const { return swapChainFramebuffers[index]; }
	VkRenderPass	getRenderPass() const noexcept { return renderPass; }
	VkImageView		getImageView(int index) const { return swapChainImageViews[index]; }
	size_t			imageCount() const noexcept { return swapChainImages.size(); }
	VkExtent2D		getSwapChainExtent() const noexcept { return swapChainExtent; }
	VkFormat		getSwapChainImageFormat() const noexcept { return swapChainImageFormat; }

	VkFormat	findDepthFormat();
	VkResult	acquireNextImage(uint32_t* imageIndex) noexcept;
	VkResult	submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

	bool	compareSwapFormats(const VulkanSwapChain& otherSwapChain) const noexcept;

	private:

	void	init();
	void	createSwapChain();
	void	createImageViews();
	void	createDepthResources();
	void	createRenderPass();
	void	createFramebuffers();
	void	createSyncVulkanObjects();

	VkSurfaceFormatKHR	chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)	const noexcept;
	VkPresentModeKHR	chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)	const noexcept;
	VkExtent2D			chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)	const noexcept;

	VkFormat	swapChainImageFormat;
	VkFormat	swapChainDepthFormat;
	VkExtent2D	swapChainExtent;

	std::vector<VkFramebuffer>	swapChainFramebuffers;
	VkRenderPass	renderPass;

	std::vector<VkImage>		depthImages;
	std::vector<VkDeviceMemory>	depthImageMemorys;
	std::vector<VkImageView>	depthImageViews;
	std::vector<VkImage>		swapChainImages;
	std::vector<VkImageView>	swapChainImageViews;

	VulkanDevice&	device;
	VkExtent2D		windowExtent;
	VkSwapchainKHR	swapChain;

	std::shared_ptr<VulkanSwapChain>	oldSwapChain;

	std::vector<VkSemaphore>	imageAvailableSemaphores;
	std::vector<VkSemaphore>	renderFinishedSemaphores;
	std::vector<VkFence>		inFlightFences;
	std::vector<VkFence>		imagesInFlight;
	
	size_t currentFrame = 0UL;
};

}
