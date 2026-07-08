#include "VulkanRenderer.hpp"

#include <chrono>
#include <random>

namespace ve {

VulkanRenderer::VulkanRenderer(VulkanWindow& window, VulkanDevice& device)
	: vulkanWindow(window), vulkanDevice(device), currentFrameIndex(0), isFrameStarted(false)
{
	recreateSwapChain();
	createCommandBuffers();
}

VulkanRenderer::~VulkanRenderer()
{
	vkFreeCommandBuffers(
		vulkanDevice.device(),
		vulkanDevice.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data()
	);
	commandBuffers.clear();
}

void	VulkanRenderer::recreateSwapChain()
{
	VkExtent2D extent = vulkanWindow.getWindowSize();

	while (extent.width == 0 || extent.height == 0)
	{
		extent = vulkanWindow.getWindowSize();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vulkanDevice.device());
	if (vulkanSwapChain == nullptr)
	{
		vulkanSwapChain = std::make_unique<VulkanSwapChain>(vulkanDevice, extent);
	}
	else
	{
		std::shared_ptr<VulkanSwapChain> oldSwapChain = std::move(vulkanSwapChain);
		vulkanSwapChain = std::make_unique<VulkanSwapChain>(vulkanDevice, extent, oldSwapChain);
		if (vulkanSwapChain->compareSwapFormats(*oldSwapChain) == false)
		{
			throw std::runtime_error("Swap chain image or depth format has changed!");
		}
	}
}

void	VulkanRenderer::createCommandBuffers()
{
	commandBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo	allocInfo{};

	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vulkanDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(vulkanDevice.device(),	&allocInfo,	commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkCommandBuffer	VulkanRenderer::beginFrame()
{
	assert(isFrameStarted == false && "Can't call beginFrame while already in progress");
	assert(currentFrameIndex < 2 && "Current frame index out of bounds");

	VkResult	result = vulkanSwapChain->acquireNextImage(&currentImageIndex);

	switch (result)
	{
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			recreateSwapChain();
			return nullptr;
		case VK_SUCCESS:
			break;
		default:
			throw std::runtime_error("failed to acquire swap chain image!");
	}
	isFrameStarted = true;

	VkCommandBuffer	commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo	beginInfo{};

	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	return commandBuffer;
}

void	VulkanRenderer::endFrame()
{
	assert(isFrameStarted == true && "Can't call endFrame while frame not in progress");

	VkCommandBuffer	commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
	VkResult	result = vulkanSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}
	isFrameStarted = false;
	currentFrameIndex = (currentFrameIndex + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void	VulkanRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameStarted == true && "Can't call beginSwapChainRenderPass if frame not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Can't begin render pass on command buffer from a different frame"
	);

	VkRenderPassBeginInfo	renderPassInfo{};

	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = vulkanSwapChain->getRenderPass();
	renderPassInfo.framebuffer = vulkanSwapChain->getFrameBuffer(currentImageIndex);
	
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = vulkanSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2>	clearValues{};

	clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport	viewport{};
	VkRect2D	scissor{{0, 0}, renderPassInfo.renderArea.extent};

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(renderPassInfo.renderArea.extent.width);
	viewport.height = static_cast<float>(renderPassInfo.renderArea.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void	VulkanRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) noexcept
{
	assert(isFrameStarted == true && "Can't call endSwapChainRenderPass if frame not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Can't end render pass on command buffer from a different frame"
	);

	vkCmdEndRenderPass(commandBuffer);
}

} // namespace ve