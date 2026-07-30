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
	unsigned char*				fontData;
	std::vector<unsigned char>	fileContent;
	int32_t						width;
	int32_t						height;
	stbtt_bakedchar 			cdata[128];
	stbtt_fontinfo				basicFontInfo;
};

struct FontModel
{
	std::shared_ptr<VulkanModel> background;
	std::shared_ptr<VulkanModel> text;
};

std::unique_ptr<ImageInfo>		loadImage(const std::string& imagePath);
std::unique_ptr<FontInfo>		loadFont(const std::string& fontPath, float fontSize, VkExtent2D sizeTexture);

class VulkanTexture
{
	public:

	VulkanTexture() = delete;
	VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType = TEXTURE_PLAIN);
	~VulkanTexture();
	VulkanTexture(const VulkanTexture& other) = delete;
	VulkanTexture(VulkanTexture&&);
	VulkanTexture& operator=(const VulkanTexture& other) = delete;

	VkDescriptorImageInfo	getDescriptorImageInfo() const noexcept;
	FontModel				getModelFromText(std::string const& text, vec2i const& origin, bool isRightAligned = false) const noexcept;

	static constexpr uint32_t defaultSizeFont = 32U;
	static constexpr VkExtent2D defaultSizeFontTexture = VkExtent2D{512U, 512U};
	static constexpr uint32_t fontPadding = 5U;

	private:

	void	createTextureImage();
	void	createTextureImageView();
	void	createTextureSampler();

	VulkanDevice&	device;
	TextureType		type;

	std::unique_ptr<ImageInfo>	imageInfo;
	std::unique_ptr<FontInfo>	fontInfo;
	VkImageCreateInfo			info{};
	VkDeviceSize				nPixels{0UL};
	VkDeviceSize				sizeOfPixel{STBTT_UNICODE_EID_UNICODE_2_0_FULL};

	VkImage			textureImage{VK_NULL_HANDLE};
	VkDeviceMemory	textureImageMemory{VK_NULL_HANDLE};
	VkImageView		textureImageView{VK_NULL_HANDLE};
	VkSampler		textureSampler{VK_NULL_HANDLE};

};

} // namespace ve
