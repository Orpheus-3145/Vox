#include "VulkanBuffer.hpp"

#include <cassert>
#include <cstring>


namespace ve {

VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDeviceSize instanceSize,	uint32_t instanceCount,
	VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment)
	:
	vulkanDevice{device}, instanceSize{instanceSize}, instanceCount{instanceCount},	usageFlags{usageFlags},
	memoryPropertyFlags{memoryPropertyFlags}
{
	alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
	bufferSize = alignmentSize * instanceCount;
	device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
}

/**
 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
VkDeviceSize	VulkanBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) noexcept
{
	if (minOffsetAlignment > 0)
	{
		instanceSize = (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

VulkanBuffer::~VulkanBuffer()
{
	unmap();
	if (this->buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(vulkanDevice.device(), buffer, nullptr);
	if (this->memory != VK_NULL_HANDLE)
		vkFreeMemory(vulkanDevice.device(), memory, nullptr);
}

VulkanBuffer::VulkanBuffer( VulkanBuffer&& other ) :
	vulkanDevice{other.vulkanDevice},
	mapped{other.mapped},
	buffer{other.buffer},
	memory{other.memory},
	bufferSize{other.bufferSize},
	instanceSize{other.instanceSize},
	instanceCount{other.instanceCount},
	alignmentSize{other.alignmentSize},
	usageFlags{other.usageFlags},
	memoryPropertyFlags{other.memoryPropertyFlags}
{
	other.mapped = nullptr;
	other.buffer = VK_NULL_HANDLE;
	other.memory = VK_NULL_HANDLE;
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
VkResult	VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) noexcept
{
	assert(buffer != VK_NULL_HANDLE && memory != VK_NULL_HANDLE && "Called map on buffer before create");
	return vkMapMemory(vulkanDevice.device(), memory, offset, size, 0, &mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void VulkanBuffer::unmap() noexcept
{
	if (mapped != nullptr)
	{
		vkUnmapMemory(vulkanDevice.device(), memory);
		mapped = nullptr;
	}
}

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void	VulkanBuffer::writeToBuffer(const void *data, VkDeviceSize size, VkDeviceSize offset) noexcept
{
	assert(mapped && data && "Cannot copy to unmapped buffer or data source null");

	unsigned char* memOffset = static_cast<unsigned char*>(mapped);
	memOffset += offset;
	if (size == VK_WHOLE_SIZE)
	{
		std::memcpy(mapped, data, bufferSize - offset);
	}
	else
	{
		std::memcpy(memOffset, data, size);
	}
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult	VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) noexcept
{
	VkMappedMemoryRange mappedRange = {};

	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(vulkanDevice.device(), 1, &mappedRange);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult	VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) noexcept
{
	VkMappedMemoryRange mappedRange = {};

	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(vulkanDevice.device(), 1, &mappedRange);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo	VulkanBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const noexcept
{
	return VkDescriptorBufferInfo{buffer, offset, size};
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void	VulkanBuffer::writeToIndex(const void *data, int32_t index) noexcept
{
	writeToBuffer(data, instanceSize, index * alignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult	VulkanBuffer::flushIndex(int32_t index) noexcept
{
	return flush(alignmentSize, index * alignmentSize);
}

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo	VulkanBuffer::descriptorInfoForIndex(int32_t index) noexcept
{
	return descriptorInfo(alignmentSize, index * alignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult	VulkanBuffer::invalidateIndex(int32_t index) noexcept
{
	return invalidate(alignmentSize, index * alignmentSize);
}

}	// namespace lve