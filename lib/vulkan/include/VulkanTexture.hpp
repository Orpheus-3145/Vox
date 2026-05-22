#pragma once

#include "VulkanObject.hpp"
#include "stb_image.h"

namespace ve {


class VulkanTexture
{
	public:

	VulkanTexture() = delete;
	VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType = TEXTURE_PLAIN);
	~VulkanTexture();
	VulkanTexture(const VulkanTexture& other) = delete;
	VulkanTexture(VulkanTexture&&);
	VulkanTexture&	operator=(const VulkanTexture& other) = delete;

	void	createTextureImage();
	void	createTextureImageView();
	void	createTextureSampler();

	VkDescriptorImageInfo	getDescriptorImageInfo() const noexcept;

	static constexpr uint32_t sizeOfPixel = sizeof(int32_t);

	private:

	ImageInfo		imageInfo;
	VkDeviceSize	nPixels;

	VkImage			textureImage{VK_NULL_HANDLE};
	VkDeviceMemory	textureImageMemory{VK_NULL_HANDLE};
	VkImageView		textureImageView{VK_NULL_HANDLE};
	VkSampler		textureSampler{VK_NULL_HANDLE};

	VkImageCreateInfo	info{};

	VulkanDevice&	device;
	TextureType		type;
};

} // namespace ve