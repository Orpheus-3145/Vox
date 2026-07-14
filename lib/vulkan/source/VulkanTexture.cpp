#include "VulkanTexture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <iostream>
#include <cstring>
#include <limits>


namespace ve {

VulkanTexture::VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType type) : 
	device(device), type(type)
{
	if (type == TEXTURE_FONT)
	{
		fontInfo = loadFont(filePath, VulkanTexture::defaultSizeFont, VulkanTexture::defaultSizeFontTexture);
	}
	else
	{
		imageInfo = loadImage(filePath);
	}

	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;

	if (type == TEXTURE_PLAIN)
	{
		info.format = VK_FORMAT_R8G8B8A8_SRGB;
		info.arrayLayers = 1;
		info.flags = 0;
		info.extent.width = static_cast<uint32_t>(imageInfo->width);
		info.extent.height = static_cast<uint32_t>(imageInfo->height);
		nPixels = info.extent.width * info.extent.height;
	}
	else if (type == TEXTURE_CUBEMAP)
	{
		info.format = VK_FORMAT_R8G8B8A8_SRGB;
		info.arrayLayers = 6;
		info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		uint32_t faceWidth  = static_cast<uint32_t>(imageInfo->width) / 4U;
		uint32_t faceHeight = static_cast<uint32_t>(imageInfo->height) / 3U;
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
	else if (type == TEXTURE_FONT)
	{
		info.format = VK_FORMAT_R8_UNORM;
		info.arrayLayers = 1;
		info.flags = 0;
		info.extent.width = static_cast<uint32_t>(fontInfo->width);
		info.extent.height = static_cast<uint32_t>(fontInfo->height);
		nPixels = info.extent.width * info.extent.height;
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
	if (imageInfo && imageInfo->imageData)
	{
		free((const_cast<unsigned char*>(imageInfo->imageData)));
	}
	if (fontInfo && fontInfo->fontData)
	{
		delete [] fontInfo->fontData;
	}
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) :
	device(other.device),
	type(other.type),
	imageInfo(std::move(other.imageInfo)),
	fontInfo(std::move(other.fontInfo)),
	info(other.info),
	nPixels(other.nPixels),
	textureImage(other.textureImage),
	textureImageMemory(other.textureImageMemory),
	textureImageView(other.textureImageView),
	textureSampler(other.textureSampler)
{
	other.imageInfo = nullptr;
	other.fontInfo = nullptr;
	other.textureImage = VK_NULL_HANDLE;
	other.textureImageView = VK_NULL_HANDLE;
	other.textureSampler = VK_NULL_HANDLE;
	other.textureImageMemory = VK_NULL_HANDLE;
}

VkDescriptorImageInfo VulkanTexture::getDescriptorImageInfo() const noexcept {
	VkDescriptorImageInfo imageInfo{};

	imageInfo.sampler = textureSampler;
	imageInfo.imageView = textureImageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return imageInfo;
}

FontModel VulkanTexture::getModelFromText(std::string const& text, vec2i const& origin, bool isRightAligned) const noexcept
{
	assert(type == TEXTURE_FONT && "Texture doesn't represent a font");

	std::vector<VulkanModel::Vertex> textVertexes, bgVertexes;
	textVertexes.resize(6 * text.size());

	uint32_t fontSize = VulkanTexture::defaultSizeFont;
	uint32_t fontPadding = VulkanTexture::fontPadding;

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();

	stbtt_aligned_quad q;
	float x = 0.0f, y = 0.0f;
	for (size_t i=0; i<text.size(); i++)
	{
		stbtt_GetBakedQuad(
			this->fontInfo->cdata,
			VulkanTexture::defaultSizeFontTexture.width,
			VulkanTexture::defaultSizeFontTexture.height,
			static_cast<int32_t>(text[i]),
			&x, &y, &q, 1
		);

		textVertexes[i * 6].pos = vec3(q.x0, q.y0, 0.0f);
		textVertexes[i * 6].textureUv = vec2(q.s0, q.t0);

		textVertexes[i * 6 + 1].pos = vec3(q.x1, q.y0, 0.0f);
		textVertexes[i * 6 + 1].textureUv = vec2(q.s1, q.t0);

		textVertexes[i * 6 + 2].pos = vec3(q.x1, q.y1, 0.0f);
		textVertexes[i * 6 + 2].textureUv = vec2(q.s1, q.t1);

		textVertexes[i * 6 + 3].pos = vec3(q.x0, q.y0, 0.0f);
		textVertexes[i * 6 + 3].textureUv = vec2(q.s0, q.t0);

		textVertexes[i * 6 + 4].pos = vec3(q.x1, q.y1, 0.0f);
		textVertexes[i * 6 + 4].textureUv = vec2(q.s1, q.t1);

		textVertexes[i * 6 + 5].pos = vec3(q.x0, q.y1, 0.0f);
		textVertexes[i * 6 + 5].textureUv = vec2(q.s0, q.t1);

		minX = std::min(minX, q.x0);
		maxX = std::max(maxX, q.x1);
	}

	float startX = origin.x;
	if (isRightAligned)
	{
		startX -= maxX;
	}
	float startY = origin.y + fontSize - fontPadding;

	for (size_t i=0; i<textVertexes.size(); i++)
	{
		textVertexes[i].pos.x += startX;
		textVertexes[i].pos.y += startY;
	}
	float scale = stbtt_ScaleForPixelHeight(&this->fontInfo->basicFontInfo, fontSize);

	int ascentRaw, descentRaw, lineGapRaw;
	stbtt_GetFontVMetrics(&this->fontInfo->basicFontInfo, &ascentRaw, &descentRaw, &lineGapRaw);

	float lineTop = -ascentRaw  * scale;
	float lineBottom = -descentRaw * scale;

	vec2 whiteUV{
		(this->fontInfo->width - 1 + 0.5f) / (float)this->fontInfo->width,
		(this->fontInfo->height - 1 + 0.5f) / (float)this->fontInfo->height
	};

	// add padding for background
	minX -= fontPadding;
	maxX += fontPadding;
	lineTop -= fontPadding;
	lineBottom += fontPadding;

	bgVertexes = std::vector<VulkanModel::Vertex>{
		VulkanModel::Vertex{vec3{minX + startX, lineTop + startY, 0.0f}, vec3(), whiteUV, 0U},
		VulkanModel::Vertex{vec3{maxX + startX, lineTop + startY, 0.0f}, vec3(), whiteUV, 0U},
		VulkanModel::Vertex{vec3{maxX + startX, lineBottom + startY, 0.0f}, vec3(), whiteUV, 0U},

		VulkanModel::Vertex{vec3{minX + startX, lineTop + startY, 0.0f}, vec3(), whiteUV, 0U},
		VulkanModel::Vertex{vec3{maxX + startX, lineBottom + startY, 0.0f}, vec3(), whiteUV, 0U},
		VulkanModel::Vertex{vec3{minX + startX, lineBottom + startY, 0.0f}, vec3(), whiteUV, 0U},
	};

	return FontModel
	{
		std::make_shared<ve::VulkanModel>(
			device,
			bgVertexes,
			std::vector<uint32_t>(),
			0U,
			ve::FONT_MODEL_LAYOUT
		),
		std::make_shared<ve::VulkanModel>(
			device,
			textVertexes,
			std::vector<uint32_t>(),
			0U,
			ve::FONT_MODEL_LAYOUT
		)
	};
}

void VulkanTexture::createTextureImage()
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
		stagingBuffer.writeToBuffer(imageInfo->imageData, nPixels * static_cast<VkDeviceSize>(VulkanTexture::sizeOfPixel));
	}
	else if (type == TEXTURE_CUBEMAP)
	{
		uint32_t faceWidth = static_cast<uint32_t>(imageInfo->width) / 4U;
		uint32_t faceHeight = static_cast<uint32_t>(imageInfo->height) / 3U;

		std::vector<vec2ui> offsets = {
			vec2ui{0 * faceWidth, 1 * faceHeight},	// left
			vec2ui{2 * faceWidth, 1 * faceHeight},	// right
			vec2ui{1 * faceWidth, 0 * faceHeight},	// down
			vec2ui{1 * faceWidth, 2 * faceHeight},	// up
			vec2ui{3 * faceWidth, 1 * faceHeight},	// back
			vec2ui{1 * faceWidth, 1 * faceHeight},	// front
		};

		uint32_t faceWidthBytes = faceWidth * VulkanTexture::sizeOfPixel;
		uint32_t faceSizeBytes  = faceWidth * faceHeight * VulkanTexture::sizeOfPixel;
		uint32_t textureWidthBytes = imageInfo->width * VulkanTexture::sizeOfPixel;

		for (uint32_t face = 0; face < 6; face++)
		{
			uint32_t x = offsets[face].x;
			uint32_t y = offsets[face].y;
			bool rotate180 = (face == 2 || face == 3); // +Y and -Y faces need 180° rotation to match Vulkan cubemap orientation
			for (uint32_t h = 0; h < faceHeight; h++)
			{
				if (!rotate180)
				{
					stagingBuffer.writeToBuffer(
						imageInfo->imageData + (h + y) * textureWidthBytes + x * VulkanTexture::sizeOfPixel,
						faceWidthBytes,
						face * faceSizeBytes + h * faceWidthBytes
					);
				}
				else
				{
					uint32_t srcH = faceHeight - 1 - h;
					for (uint32_t w = 0; w < faceWidth; w++)
					{
						uint32_t srcW = faceWidth - 1 - w;
						stagingBuffer.writeToBuffer(
							imageInfo->imageData + (srcH + y) * textureWidthBytes + (x + srcW) * VulkanTexture::sizeOfPixel,
							VulkanTexture::sizeOfPixel,
							face * faceSizeBytes + h * faceWidthBytes + w * VulkanTexture::sizeOfPixel
						);
					}
				}
			}
		}
	}
	else if (type == TEXTURE_FONT)
	{
		stagingBuffer.writeToBuffer(fontInfo->fontData, nPixels * static_cast<VkDeviceSize>(VulkanTexture::sizeOfPixel));
	}
	stagingBuffer.flush();

	if (type == TEXTURE_FONT)
	{
		delete [] fontInfo->fontData;
		fontInfo->fontData = nullptr;
	}
	else
	{
		free((const_cast<unsigned char*>(imageInfo->imageData)));
		imageInfo->imageData = nullptr;
	}

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
		static_cast<uint32_t>((type == TEXTURE_FONT) ? fontInfo->width : imageInfo->width),
		static_cast<uint32_t>((type == TEXTURE_FONT) ? fontInfo->height : imageInfo->height),
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

void VulkanTexture::createTextureImageView()
{
	textureImageView = device.createImageView(
		textureImage,
		info.format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		info.arrayLayers,
		type
	);
}

void VulkanTexture::createTextureSampler()
{
	VkSamplerCreateInfo	samplerInfo{};

	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.flags = 0;
	if (type == TEXTURE_CUBEMAP)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	else
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

	if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}


std::unique_ptr<ImageInfo> loadImage(const std::string& imagePath)
{
	std::unique_ptr<ImageInfo> imageInfo = std::make_unique<ImageInfo>();

	imageInfo->imageData = stbi_load(imagePath.c_str(), &imageInfo->width, &imageInfo->height, &imageInfo->channels, STBI_rgb_alpha);
	if (imageInfo->imageData == nullptr)
	{
		throw std::runtime_error("Failed to load image: " + imagePath);
	}
	std::cout << "Loaded image: " << imagePath << " (" << imageInfo->width << "x" << imageInfo->height << ", " << imageInfo->channels << " channels)" << std::endl;
	return imageInfo;
}

std::unique_ptr<FontInfo> loadFont(const std::string& fontPath, float fontSize, VkExtent2D sizeTexture)
{
	std::unique_ptr<FontInfo> fontInfo = std::make_unique<FontInfo>();
	// size of the atlas in byte is: nGliphs * areaGliph ( = widthGliph * heightGliph = sizeGliph^2)
	// assuming the atlas to be a square, width = height = sqrt(sizeAtlas) = sqrt(nGliphs * sizeGliph^2) =
	// = sqrt(nGliphs) * sizeGliph [nGliphs = 96, sizeGliph (=fontSize) = 32] ~= 314 [rounded to 512]
	fontInfo->width = sizeTexture.width;
	fontInfo->height = sizeTexture.height;
	fontInfo->fontData = new unsigned char[fontInfo->width * fontInfo->height];

	int32_t count = 2;
	std::vector<unsigned char> fileContent = readFile(fontPath);
	while (stbtt_BakeFontBitmap(fileContent.data(), 0, fontSize, fontInfo->fontData, fontInfo->width, fontInfo->height, 0, 128, fontInfo->cdata) <= 0)
	{
		delete [] fontInfo->fontData;
		if (count < 0)
		{
			throw std::runtime_error("Failed to load font: " + fontPath);
		}
		fontInfo->width *= 2;
		fontInfo->height *= 2;
		fontInfo->fontData = new unsigned char[fontInfo->width * fontInfo->height];
		count--;
	}

	int32_t whitePixelX = fontInfo->width - 1;
	int32_t whitePixelY = fontInfo->height - 1;
	fontInfo->fontData[whitePixelY * fontInfo->width + whitePixelX] = 255;
	if (!stbtt_InitFont(&fontInfo->basicFontInfo, fileContent.data(), stbtt_GetFontOffsetForIndex(fileContent.data(), 0)))
	{
		delete [] fontInfo->fontData;
		fontInfo->fontData = nullptr;
		throw std::runtime_error("Failed to load font: " + fontPath);
	}

	std::cout << "Loaded font: " << fontPath << std::endl;
	return fontInfo;
}

} // namespace ve
