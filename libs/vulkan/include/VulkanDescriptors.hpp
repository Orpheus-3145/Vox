#pragma once

#include <memory>
#include <vector>
#include <map>

#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanBuffer.hpp"


namespace ve {

class VulkanBindingSet;

class BindInfo
{
	public:
		BindInfo( void ) = delete;
		BindInfo( VkDescriptorSetLayoutBinding const& vkInfo ) noexcept : vkInfo{vkInfo} {};
		virtual ~BindInfo( void ) = 0;
		BindInfo( BindInfo const& other ) = default;
		BindInfo( BindInfo&& other ) = default;
		BindInfo& operator=( BindInfo const& other ) = default;
		BindInfo& operator=( BindInfo& other ) = default;
		
		uint32_t	getBinding( void ) const noexcept { return this->vkInfo.binding; }
		uint32_t	getNitems( void ) const noexcept { return this->vkInfo.descriptorCount; }

	protected:
		VkDescriptorSetLayoutBinding	vkInfo;

	friend class VulkanBindingSet;
};

class UniformBindInfo : public BindInfo
{
	public:
		UniformBindInfo( VkDescriptorSetLayoutBinding const& vkInfo, uint32_t bufferSize, BufferType bufferType)
			:	BindInfo(vkInfo), bufferType{bufferType}
		{
			this->bufferSizes.push_back(bufferSize);
		}

		UniformBindInfo( VkDescriptorSetLayoutBinding const& vkInfo, IndexVector const& bufferSizes, BufferType bufferType) noexcept
			:	BindInfo(vkInfo), bufferSizes{bufferSizes}, bufferType{bufferType} {}

		IndexVector const&	getBufferSizes( void ) const noexcept { return bufferSizes; }
		BufferType						getBufferType( void ) const noexcept { return bufferType; }

	private:
		IndexVector		bufferSizes;
		BufferType					bufferType;
};

class SamplerBindInfo : public BindInfo
{
	public:
		SamplerBindInfo( VkDescriptorSetLayoutBinding const& vkInfo, std::string const& texturePath, TextureType textureType)
			:	BindInfo(vkInfo)
		{
			this->texturePaths.push_back(texturePath);
			this->textureTypes.push_back(textureType);
		}

		SamplerBindInfo( VkDescriptorSetLayoutBinding const& vkInfo, std::vector<std::string> const& texturePaths, std::vector<TextureType> const& textureTypes) noexcept
			:	BindInfo(vkInfo), texturePaths{texturePaths}, textureTypes{textureTypes} {}

		std::vector<std::string> const&	getTexturePaths( void ) const noexcept { return texturePaths; }
		std::vector<TextureType> const&	getTextureTypes( void ) const noexcept { return textureTypes; }

	private:
		std::vector<std::string>	texturePaths;
		std::vector<TextureType>	textureTypes;
};

class VulkanBindingSet
{
	public:
		VulkanBindingSet( void ) noexcept : id{VulkanBindingSet::ID_INSTANCE++} {}
		VulkanBindingSet( VulkanBindingSet const& other ) = delete;
		VulkanBindingSet( VulkanBindingSet&& other ) = default;
		VulkanBindingSet& operator=( VulkanBindingSet const& other ) = delete;
		VulkanBindingSet& operator=( VulkanBindingSet&& other ) = delete;

		VulkanBindingSet&	addBufferBinding( uint32_t binding, VkShaderStageFlags stage, uint32_t bufferSize, BufferType bufferType = BUFFER_UNIFORM );
		VulkanBindingSet&	addSamplerBinding( uint32_t binding, VkShaderStageFlags stage, std::string const& texturePath, TextureType textureInfo = TEXTURE_PLAIN );

		VulkanBindingSet&	addBufferArrayBinding( uint32_t binding, VkShaderStageFlags stage, IndexVector const& sizes, BufferType bufferType = BUFFER_UNIFORM );
		VulkanBindingSet&	addSamplerArrayBinding( uint32_t binding, VkShaderStageFlags stage, std::vector<std::string> const& texturePaths, std::vector<TextureType> types );

		uint32_t										getId( void ) const noexcept { return this->id; }
		std::vector<std::unique_ptr<BindInfo>> const&	getBindingData( void ) const noexcept { return this->bindings; }
		std::vector<VkDescriptorSetLayoutBinding>		getVkBindingData( void ) const noexcept;

	private:
		static uint32_t ID_INSTANCE;

		std::vector<std::unique_ptr<BindInfo>>	bindings;
		const uint32_t							id;
};

class VulkanDescriptorSet;

class VulkanDescriptorSetFactory
{
	public:
		VulkanDescriptorSetFactory( void ) = delete;
		VulkanDescriptorSetFactory( VulkanDevice& vulkanDevice) noexcept : vulkanDevice{vulkanDevice} {}
		~VulkanDescriptorSetFactory( void ) noexcept;
		VulkanDescriptorSetFactory( VulkanDescriptorSetFactory const& other) = delete;
		VulkanDescriptorSetFactory( VulkanDescriptorSetFactory& other) noexcept;
		VulkanDescriptorSetFactory& operator=( VulkanDescriptorSetFactory const& other ) = delete;
		VulkanDescriptorSetFactory& operator=( VulkanDescriptorSetFactory& other ) = delete;

		VulkanDescriptorSetFactory&	addPoolSize( VkDescriptorType type, uint32_t count = 1U );
		VulkanDescriptorSetFactory&	addBufferPoolSize( uint32_t count = 1U );
		VulkanDescriptorSetFactory&	addSsboPoolSize( uint32_t count = 1U );
		VulkanDescriptorSetFactory&	addSamplerPoolSize( uint32_t count = 1U );

		VulkanDescriptorSetFactory&	setPoolFlags( VkDescriptorPoolCreateFlags flags ) noexcept;
		VulkanDescriptorSetFactory&	setMaxSets( uint32_t count ) noexcept;

		VulkanDescriptorSetFactory& createPool( void );
		VulkanDescriptorSetFactory&	resetPool( void ) noexcept;

		std::unique_ptr<VulkanDescriptorSet>	createDescriptorSet( VulkanBindingSet const& bindings );

	private:
		void	addNewLayout( VulkanBindingSet const& bindings );

		VulkanDevice&				vulkanDevice;
		VkDescriptorPoolCreateFlags	poolFlags{0U};
		uint32_t					maxSets{0U};

		std::map<VkDescriptorType,uint32_t>		countTypes{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0U},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0U},
		};
		VkDescriptorPool						descriptorPool{VK_NULL_HANDLE};
		// descriptor set layouts are linked to a specific binding, the map is used for fast lookup
		// the vector is instead returned to be given to pipelines creation
		std::map<uint32_t,VkDescriptorSetLayout>	existingLayouts;
		std::vector<VkDescriptorSetLayout>			descriptorSetlayouts;
};


class VulkanDescriptor;
class VulkanBufferDescriptor;
class VulkanSamplerDescriptor;

class VulkanDescriptorSet
{
	public:
		VulkanDescriptorSet( void ) = delete;
		VulkanDescriptorSet(
			VulkanDevice& 			vulkanDevice,
			VkDescriptorSetLayout	descriptorSetLayout,
			VkDescriptorPool		descriptorPool,
			VulkanBindingSet const&	bindings
		);
		~VulkanDescriptorSet( void ) = default;
		VulkanDescriptorSet( VulkanDescriptorSet const& other ) = delete;
		VulkanDescriptorSet( VulkanDescriptorSet&& other ) = delete;
		VulkanDescriptorSet& operator=( VulkanDescriptorSet const& other ) = delete;
		VulkanDescriptorSet& operator=( VulkanDescriptorSet&& other ) = delete;

		void	updateDescriptor( uint32_t binding, void const* data, uint32_t index = 0U ) noexcept;
		void	bindSet( VkCommandBuffer commandBuffer, VulkanPipeline const& pipeline, uint32_t setIndex ) noexcept;

		VkDescriptorSetLayout			getLayout( void ) const noexcept { return this->descriptorSetLayout; };
		VulkanBufferDescriptor const*	getBufferDescriptor(uint32_t binding) const noexcept;
		VulkanSamplerDescriptor const*	getSamplerDescriptor(uint32_t binding) const noexcept;

	private:
		VkDescriptorSetLayout	descriptorSetLayout{VK_NULL_HANDLE};
		VkDescriptorSet			descriptorSet{VK_NULL_HANDLE};

		std::map<uint32_t,std::unique_ptr<VulkanDescriptor>>	descriptors{};

		friend class VulkanDescriptorSetFactory;
};

class VulkanDescriptor
{
	public:
		VulkanDescriptor( BindInfo const& binding ) : binding{binding.getBinding()} {}
		VulkanDescriptor( void ) = delete;
		virtual ~VulkanDescriptor( void ) {};
		VulkanDescriptor( VulkanDescriptor const& other ) = delete;
		VulkanDescriptor( VulkanDescriptor&& other ) = default;
		VulkanDescriptor& operator=( VulkanDescriptor const& other ) = delete;
		VulkanDescriptor& operator=( VulkanDescriptor&& other ) = delete;

		virtual void	update( void const* data, uint32_t index = 0U ) noexcept = 0;

	protected:
		uint32_t	binding;
};

class VulkanBufferDescriptor : public VulkanDescriptor
{
	public:
		VulkanBufferDescriptor( UniformBindInfo const& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet );

		void	update( void const* data, uint32_t index = 0U ) noexcept override;

	private:
		std::vector<std::unique_ptr<VulkanBuffer>>	buffers{};
};

class VulkanSamplerDescriptor : public VulkanDescriptor
{
	public:
		VulkanSamplerDescriptor( SamplerBindInfo const& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet );

		void	update( void const* data, uint32_t index = 0U ) noexcept override;

		UIvertexes	getUIvertexes(std::string const& text, vec2i const& origin, uint32_t index = 0U, bool isRightAligned = false) const noexcept;

	private:
		std::vector<std::unique_ptr<VulkanTexture>>	textures{};
};

}  // namespace ve
