#pragma once

#include "VulkanDevice.hpp"


namespace ve {

class VulkanBuffer
{
	public:

	VulkanBuffer(
		VulkanDevice& device,
		VkDeviceSize instanceSize,
		uint32_t instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment = 1);
	~VulkanBuffer();

	VulkanBuffer() = delete;
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&&);
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VkResult	map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	void		unmap() noexcept;

	void		writeToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	VkResult	flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	
	VkDescriptorBufferInfo	descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const noexcept;
	VkResult				invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;

	void		writeToIndex(const void* data, int index) noexcept;
	VkResult	flushIndex(int index) noexcept;
	VkDescriptorBufferInfo	descriptorInfoForIndex(int index) noexcept;
	VkResult	invalidateIndex(int index) noexcept;

	VkBuffer		getBuffer() const noexcept { return buffer; }
	void*			getMappedMemory() const noexcept { return mapped; }
	uint32_t		getInstanceCount() const noexcept { return instanceCount; }
	VkDeviceSize	getInstanceSize() const noexcept { return instanceSize; }
	VkDeviceSize	getAlignmentSize() const noexcept { return instanceSize; }
	VkDeviceSize	getBufferSize() const noexcept { return bufferSize; }

	VkBufferUsageFlags		getUsageFlags() const noexcept { return usageFlags; }
	VkMemoryPropertyFlags	getMemoryPropertyFlags() const noexcept { return memoryPropertyFlags; }

	private:

	static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) noexcept;

	VulkanDevice&	vulkanDevice;
	void*			mapped = nullptr;
	VkBuffer		buffer = VK_NULL_HANDLE;
	VkDeviceMemory	memory = VK_NULL_HANDLE;

	VkDeviceSize	bufferSize;
	VkDeviceSize	instanceSize;
	uint32_t		instanceCount;
	VkDeviceSize	alignmentSize;

	VkBufferUsageFlags		usageFlags;
	VkMemoryPropertyFlags	memoryPropertyFlags;
};

}	// namespace lve