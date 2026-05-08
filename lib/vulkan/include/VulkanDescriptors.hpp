#pragma once

#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"

#include <memory>
#include <vector>
#include <map>


namespace ve {

struct UniformBinding
{
	uint32_t		binding{0U};
	ui32			bufferSize{0U};
};

struct SamplerBinding
{
	uint32_t		binding{0U};
	std::string		texturePath;
	TextureType		textureType{TEXTURE_PLAIN};
};

class VulkanBindingSet
{
	public:
		VulkanBindingSet( void ) : id{VulkanBindingSet::ID_INSTANCE++} {}
		VulkanBindingSet( VulkanBindingSet const& other ) = delete;
		VulkanBindingSet( VulkanBindingSet&& other ) = delete;
		VulkanBindingSet& operator=( VulkanBindingSet const& other ) = delete;
		VulkanBindingSet& operator=( VulkanBindingSet&& other ) = delete;

		VulkanBindingSet&	addUniformBinding( uint32_t binding, VkShaderStageFlags stage, uint32_t bufferSize, uint32_t count = 1U );
		VulkanBindingSet&	addSamplerBinding( uint32_t binding, VkShaderStageFlags stage, std::string const& texturePath, TextureType textureInfo, uint32_t count = 1U );

		ui32								getId( void ) const noexcept { return this->id; }
		std::vector<UniformBinding>	const&	getUniformBindings( void ) const noexcept { return this->uniformBindings; }
		std::vector<SamplerBinding>	const&	getSamplerBindings( void ) const noexcept { return this->samplerBindings; }

		VkDescriptorSetLayoutBinding const*	getBindingData( void ) const noexcept;
		ui32								getBindingDataSize( void ) const noexcept;

	private:
		void	addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count);

		static ui32 ID_INSTANCE;

		std::vector<VkDescriptorSetLayoutBinding>	bindings;
		std::vector<UniformBinding>					uniformBindings;
		std::vector<SamplerBinding>					samplerBindings;
		ui32										id;
};

class VulkanDescriptorSet;

class VulkanDescriptorSetFactory
{
	public:
		VulkanDescriptorSetFactory( void ) = delete;
		VulkanDescriptorSetFactory( VulkanDevice& vulkanDevice) : vulkanDevice{vulkanDevice} {}
		~VulkanDescriptorSetFactory( void );
		VulkanDescriptorSetFactory( VulkanDescriptorSetFactory const& other) = delete;
		VulkanDescriptorSetFactory& operator=( VulkanDescriptorSetFactory const& other ) = delete;

		VulkanDescriptorSetFactory&	addPoolSize( VkDescriptorType type, uint32_t count = 1U );
		VulkanDescriptorSetFactory&	addBufferPoolSize( uint32_t count = 1U );
		VulkanDescriptorSetFactory&	addSamplerPoolSize( uint32_t count = 1U );

		VulkanDescriptorSetFactory&	setPoolFlags( VkDescriptorPoolCreateFlags flags ) noexcept;
		VulkanDescriptorSetFactory&	setMaxSets( uint32_t count ) noexcept;
		VulkanDescriptorSetFactory&	setFramesInFlight( uint32_t framesInFlight ) noexcept;

		VulkanDescriptorSetFactory& createPool( void );
		VulkanDescriptorSetFactory&	resetPool( void ) noexcept;

		std::unique_ptr<VulkanDescriptorSet>	createDescriptorSet( VulkanBindingSet const& bindings );

		std::vector<VkDescriptorSetLayout>	getDescriptorSetLayout( void ) const noexcept { return this->descriptorSetlayouts; }

	private:
		void	addNewLayout( VkDescriptorSetLayoutBinding const* bindingData, ui32 size, ui32 idBinding );

		VulkanDevice&				vulkanDevice;
		uint32_t					framesInFlight{1U};
		VkDescriptorPoolCreateFlags	poolFlags{0U};
		uint32_t					maxSets{0U};

		std::map<VkDescriptorType,ui32>		countTypes{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0U},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0U},
		};
		VkDescriptorPool						descriptorPool{VK_NULL_HANDLE};
		// descriptor set layouts are linked to a specific binding, the map is used for fast lookup
		// the vector is instead returned to be given to pipelines creation
		std::map<ui32,VkDescriptorSetLayout>	existingLayouts;
		std::vector<VkDescriptorSetLayout>		descriptorSetlayouts;
};

class VulkanDescriptorSet
{
	public:
		VulkanDescriptorSet( void ) = delete;
		VulkanDescriptorSet(
			VulkanDevice& 			vulkanDevice,
			uint32_t				framesInFlight,
			VkDescriptorSetLayout	descriptorSetLayout,
			VkDescriptorPool		descriptorPool,
			VulkanBindingSet const&	bindings
		);
		VulkanDescriptorSet( VulkanDescriptorSet const& other ) = delete;
		VulkanDescriptorSet( VulkanDescriptorSet&& other );
		VulkanDescriptorSet& operator=( VulkanDescriptorSet const& other ) = delete;
		VulkanDescriptorSet& operator=( VulkanDescriptorSet&& other ) = delete;

		void	setCurrentFrame( uint32_t frame ) noexcept;
		void	updateUniform( int32_t binding, void const* data ) noexcept;
		void	updateUniformAll( int32_t binding, void const* data ) noexcept;
		void	bindSet( VkCommandBuffer commandBuffer, VulkanPipeline const& pipeline, uint32_t setIndex ) noexcept;

	private:
		void	addBufferDescriptor( UniformBinding const& bindData ) noexcept;
		void	addSamplerDescriptor( SamplerBinding const& bindData ) noexcept;

		VulkanDevice&			vulkanDevice;
		uint32_t				framesInFlight;
		uint32_t				currentFrame{0U};

		std::vector<VkDescriptorSet>									descriptorSets{};
		std::map<int32_t,std::vector<std::unique_ptr<VulkanBuffer>>>	buffers{};
		std::map<int32_t,std::unique_ptr<VulkanTexture>>				textures{};

		friend class VulkanDescriptorSetFactory;
};

}  // namespace ve
