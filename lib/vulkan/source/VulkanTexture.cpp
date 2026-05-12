#include "VulkanTexture.hpp"
#include <iostream>

namespace ve {

VulkanTexture::VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType type) : 
	device(device), type(type)
{
	textureImage = VK_NULL_HANDLE;
	textureImageView = VK_NULL_HANDLE;
	textureSampler = VK_NULL_HANDLE;

	imageInfo = loadImage(filePath);
	if (imageInfo.imageData == nullptr)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.format = VK_FORMAT_R8G8B8A8_SRGB;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	if (type == TEXTURE_PLAIN) {
		info.arrayLayers = 1;
		info.flags = 0;
		info.extent.width = static_cast<uint32_t>(imageInfo.width);
		info.extent.height = static_cast<uint32_t>(imageInfo.height);
		nPixels = info.extent.width * info.extent.height;
	} else if (type == TEXTURE_CUBEMAP) {
		info.arrayLayers = 6;
		info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		uint32_t faceWidth  = static_cast<uint32_t>(imageInfo.width) / 4U;
		uint32_t faceHeight = static_cast<uint32_t>(imageInfo.height) / 3U;
		if (faceWidth == faceHeight)
		{
			info.extent.width = faceWidth;
			info.extent.height = faceHeight;
		}
		else
		{
			info.extent.width = std::min(faceWidth, faceHeight);
			info.extent.height = info.extent.width;
		}
		nPixels = info.extent.width * info.extent.height * 6;
	}

	createTextureImage();
	createTextureImageView();
	createTextureSampler();
}

VulkanTexture::~VulkanTexture()
{
	if (textureImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device.device(), textureImageView, nullptr);
	}
	if (textureSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(device.device(), textureSampler, nullptr);
	}
	if (textureImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(device.device(), textureImage, nullptr);
	}
	if (textureImageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device.device(), textureImageMemory, nullptr);
	}
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) :
	imageInfo(other.imageInfo),
	nPixels(other.nPixels),
	textureImage(other.textureImage),
	textureImageMemory(other.textureImageMemory),
	textureImageView(other.textureImageView),
	textureSampler(other.textureSampler),
	info(other.info),
	device(other.device)
{
	other.imageInfo = {};
	other.nPixels = 0;
	other.textureImage = VK_NULL_HANDLE;
	other.textureImageView = VK_NULL_HANDLE;
	other.textureSampler = VK_NULL_HANDLE;
	other.textureImageMemory = VK_NULL_HANDLE;
}

void	VulkanTexture::createTextureImage()
{
	VulkanBuffer	stagingBuffer(
		device,
		VulkanTexture::sizeOfPixel,
		nPixels,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);
	stagingBuffer.map();

	if (type == TEXTURE_PLAIN)
	{
		stagingBuffer.writeToBuffer(imageInfo.imageData, nPixels * static_cast<VkDeviceSize>(VulkanTexture::sizeOfPixel));
	}
	else if (type == TEXTURE_CUBEMAP)
	{
		uint32_t faceWidth = static_cast<uint32_t>(imageInfo.width) / 4U;
		uint32_t faceHeight = static_cast<uint32_t>(imageInfo.height) / 3U;
		uint32_t paddingFace = std::abs(static_cast<int32_t>(faceHeight) - static_cast<int32_t>(faceWidth));
		if (paddingFace != 0U) {
			faceWidth = std::min(faceWidth, faceHeight);
			faceHeight = faceWidth;
		}

		// order of the faces matters!
		std::vector<vec2ui> offsets = {
			vec2ui{0 * faceWidth + paddingFace, 1 * faceHeight + paddingFace},	// left
			vec2ui{2 * faceWidth + paddingFace, 1 * faceHeight + paddingFace},	// right
			vec2ui{1 * faceWidth + paddingFace, 0 * faceHeight + paddingFace},	// down
			vec2ui{1 * faceWidth + paddingFace, 2 * faceHeight + paddingFace},	// up
			vec2ui{3 * faceWidth + paddingFace, 1 * faceHeight + paddingFace},	// back
			vec2ui{1 * faceWidth + paddingFace, 1 * faceHeight + paddingFace},	// front
		};

		uint32_t faceWidthBytes  = (faceWidth - 2 * paddingFace) * VulkanTexture::sizeOfPixel;
		uint32_t faceSizeBytes  = (faceWidth - paddingFace) * (faceHeight - paddingFace) * VulkanTexture::sizeOfPixel;
		uint32_t textureWidthBytes = (imageInfo.width - 2 * paddingFace) * VulkanTexture::sizeOfPixel;
		for (uint32_t face = 0; face < 6; face++)
		{
			uint32_t x = offsets[face].x;
			uint32_t y = offsets[face].y;
			for (uint32_t h = 0; h < faceHeight; h++)
			{
				stagingBuffer.writeToBuffer(
					imageInfo.imageData + (h + y) * textureWidthBytes + x * VulkanTexture::sizeOfPixel,
					faceWidthBytes,
					face * faceSizeBytes + h * faceWidthBytes
				);
			}
		}
	}
	stagingBuffer.flush();

	free((const_cast<unsigned char*>(imageInfo.imageData)));
	imageInfo.imageData = nullptr;

	device.createImageWithInfo(
		info,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory
	);
	device.transitionImageLayout(
		textureImage,
		info.format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		info.arrayLayers
	);
	device.copyBufferToImage(
		stagingBuffer.getBuffer(),
		textureImage,
		static_cast<uint32_t>(imageInfo.width),
		static_cast<uint32_t>(imageInfo.height),
		info.arrayLayers,
		type
	);
	device.transitionImageLayout(
		textureImage,
		info.format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		info.arrayLayers
	);
}

void	VulkanTexture::createTextureImageView()
{
	textureImageView = device.createImageView(
		textureImage,
		info.format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		info.arrayLayers,
		type
	);
}

VkDescriptorImageInfo	VulkanTexture::getDescriptorImageInfo() const noexcept {
	VkDescriptorImageInfo imageInfo{};

	imageInfo.sampler = textureSampler;
	imageInfo.imageView = textureImageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return imageInfo;
}

void	VulkanTexture::createTextureSampler()
{
	VkSamplerCreateInfo	samplerInfo{};
	
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;		// VK_FILTER_NEAREST for cubemaps (adds padding) but result is ugly
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	if (type == TEXTURE_PLAIN)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
	else if (type == TEXTURE_CUBEMAP)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	samplerInfo.maxAnisotropy = 1.0f;

	if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

} // namespace ve