#pragma once

#include "VulkanObject.hpp"

#include "stb_truetype.h"


namespace ve {

struct ImageInfo
{
	unsigned char*	imageData;
	int32_t			width;
	int32_t			height;
	int32_t			channels;
};

struct FontInfo
{
	unsigned char*	fontData;
	int32_t			width;
	int32_t			height;
	stbtt_bakedchar cdata[128];
};

std::unique_ptr<ImageInfo>		loadImage(const std::string& imagePath);
std::unique_ptr<FontInfo>		loadFont(const std::string& fontPath);

class VulkanTexture
{
	public:

	VulkanTexture() = delete;
	VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType = TEXTURE_PLAIN);
	~VulkanTexture();
	VulkanTexture(const VulkanTexture& other) = delete;
	VulkanTexture(VulkanTexture&&);
	VulkanTexture&	operator=(const VulkanTexture& other) = delete;

	VkDescriptorImageInfo			getDescriptorImageInfo() const noexcept;
	std::unique_ptr<VulkanModel>	getModelFromText(std::string const& text) const noexcept;

	static constexpr uint32_t sizeOfPixel = sizeof(int32_t);
	static constexpr uint32_t sizeOfFontPixel = sizeof(int8_t);

	private:

	void	createTextureImage();
	void	createTextureImageView();
	void	createTextureSampler();

	std::unique_ptr<ImageInfo>	imageInfo;
	std::unique_ptr<FontInfo>	fontInfo;
	VkDeviceSize				nPixels;

	VkImage			textureImage{VK_NULL_HANDLE};
	VkDeviceMemory	textureImageMemory{VK_NULL_HANDLE};
	VkImageView		textureImageView{VK_NULL_HANDLE};
	VkSampler		textureSampler{VK_NULL_HANDLE};

	VkImageCreateInfo	info{};

	VulkanDevice&	device;
	TextureType		type;
};

} // namespace ve
