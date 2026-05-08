#include "VulkanDescriptors.hpp"
#include "VulkanTexture.hpp"

#include <cassert>


namespace ve {

ui32 VulkanBindingSet::ID_INSTANCE = 0U;

VulkanBindingSet&	VulkanBindingSet::addUniformBinding( uint32_t binding, VkShaderStageFlags stage, uint32_t bufferSize, uint32_t count )
{
	this->addBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage, count);

	UniformBinding newBinding{};
	newBinding.binding = binding;
	newBinding.bufferSize = bufferSize;
	this->uniformBindings.push_back(newBinding);

	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addSamplerBinding( uint32_t binding, VkShaderStageFlags stage, std::string const& texturePath, TextureType textureType, uint32_t count )
{
	this->addBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, count);

	SamplerBinding newBinding{};
	newBinding.binding = binding;
	newBinding.texturePath = texturePath;
	newBinding.textureType = textureType;
	this->samplerBindings.push_back(newBinding);

	return *this;
}

VkDescriptorSetLayoutBinding const* VulkanBindingSet::getBindingData( void ) const noexcept
{
	return this->bindings.data();
}

ui32 VulkanBindingSet::getBindingDataSize( void ) const noexcept
{
	return this->bindings.size();
}

void VulkanBindingSet::addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count)
{
	for (VkDescriptorSetLayoutBinding const& bindingInfo : this->bindings)
	{
		if (bindingInfo.binding == binding)
		{
			throw std::runtime_error("binding for descriptor already set");
		}
	}

	VkDescriptorSetLayoutBinding bindingInfo{};
	bindingInfo.binding = binding;
	bindingInfo.descriptorType = type;
	bindingInfo.descriptorCount = count;
	bindingInfo.stageFlags = stage;
	this->bindings.push_back(bindingInfo);
}


VulkanDescriptorSetFactory::~VulkanDescriptorSetFactory( void )
{
	for ( VkDescriptorSetLayout layoutSet : this->descriptorSetlayouts)
	{
		vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), layoutSet, nullptr);
	}
	this->existingLayouts.clear();
	this->descriptorSetlayouts.clear();

	if (this->descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, nullptr);
	}
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addPoolSize(VkDescriptorType type, uint32_t count)
{
	assert( this->countTypes.count(type) > 0 and "only descriptor type supported: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");
	this->countTypes[type] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addBufferPoolSize(uint32_t count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addSamplerPoolSize(uint32_t count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::setPoolFlags(VkDescriptorPoolCreateFlags flags) noexcept
{
	this->poolFlags |= flags;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::setMaxSets(uint32_t count) noexcept
{
	this->maxSets = count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::setFramesInFlight( uint32_t framesInFlight ) noexcept
{
	this->framesInFlight = framesInFlight;
	return *this;
}

VulkanDescriptorSetFactory& VulkanDescriptorSetFactory::createPool( void )
{
	assert(this->maxSets > 0 && this->framesInFlight > 0 && "invalid number (<= 0) of sets or frames in flight");

	std::vector<VkDescriptorPoolSize>	poolSizes{};
	if (this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] * this->framesInFlight
		});
	}
	if (this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] * this->framesInFlight
		});
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = this->maxSets * this->framesInFlight;
	poolInfo.flags = this->poolFlags;

	if (vkCreateDescriptorPool(this->vulkanDevice.device(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
	return *this;
}

VulkanDescriptorSetFactory& VulkanDescriptorSetFactory::resetPool( void ) noexcept
{
	for ( VkDescriptorSetLayout layoutSet : this->descriptorSetlayouts)
	{
		vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), layoutSet, nullptr);
	}
	this->existingLayouts.clear();
	this->descriptorSetlayouts.clear();

	if (this->descriptorPool != VK_NULL_HANDLE)
	{
		// automatically also clears every descriptor set created by this pool
		vkResetDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, 0);
		this->descriptorPool = VK_NULL_HANDLE;
	}

	this->framesInFlight = 1U;
	this->poolFlags = 0U;
	this->maxSets = 0U;
	this->countTypes.clear();
	return *this;
}

std::unique_ptr<VulkanDescriptorSet> VulkanDescriptorSetFactory::createDescriptorSet( VulkanBindingSet const& bindings )
{
	assert(this->descriptorPool != VK_NULL_HANDLE && "pool not created");

	if(this->maxSets == 0U)
	{
		throw std::runtime_error("not enough sets left from the pool");
	}
	this->maxSets--;

	std::vector<UniformBinding> const& uniforms = bindings.getUniformBindings();
	std::vector<SamplerBinding> const& samplers = bindings.getSamplerBindings();
	if ((uniforms.size() + samplers.size()) == 0U)
	{
		throw std::runtime_error("no binding set");
	}

	if(this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] < uniforms.size())
	{
		throw std::runtime_error("not enough uniform descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] -= uniforms.size();

	if(this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] < samplers.size())
	{
		throw std::runtime_error("not enough sampler descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] -= samplers.size();

	// create a new descLayout if none is found linked to this binding
	if (this->existingLayouts.count(bindings.getId()) == 0U)
	{
		this->addNewLayout(bindings.getBindingData(), bindings.getBindingDataSize(), bindings.getId());
	}

	return std::make_unique<VulkanDescriptorSet>(
		this->vulkanDevice,
		this->framesInFlight,
		this->existingLayouts[bindings.getId()],
		this->descriptorPool,
		bindings
	);
}

void VulkanDescriptorSetFactory::addNewLayout( VkDescriptorSetLayoutBinding const* bindingData, ui32 size, ui32 idBinding )
{
	VkDescriptorSetLayout			newLayout;
	VkDescriptorSetLayoutCreateInfo	descriptorSetLayoutInfo{};

	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = size;
	descriptorSetLayoutInfo.pBindings = bindingData;

	if (vkCreateDescriptorSetLayout(
		this->vulkanDevice.device(),
		&descriptorSetLayoutInfo,
		nullptr,
		&newLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout");
	}
	this->descriptorSetlayouts.push_back(newLayout);
	this->existingLayouts[idBinding] = this->descriptorSetlayouts.back();
}


VulkanDescriptorSet::VulkanDescriptorSet(
	VulkanDevice&			vulkanDevice,
	uint32_t				framesInFlight,
	VkDescriptorSetLayout	descriptorSetLayout,
	VkDescriptorPool		descriptorPool,
	VulkanBindingSet const&	bindings
) :
	vulkanDevice{vulkanDevice},
	framesInFlight{framesInFlight}
{
	// if N descriptor sets are created then also N setLayouts are necessary
	std::vector<VkDescriptorSetLayout> layouts(this->framesInFlight, descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = layouts.data();
	allocInfo.descriptorSetCount = this->framesInFlight;

	this->descriptorSets.resize(this->framesInFlight);
	if (vkAllocateDescriptorSets(this->vulkanDevice.device(), &allocInfo, this->descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set");
	}

	for (UniformBinding const& binding : bindings.getUniformBindings())
	{
		this->buffers[binding.binding].resize(this->framesInFlight);
		this->addBufferDescriptor(binding);
	}
	for (SamplerBinding const& binding : bindings.getSamplerBindings())
	{
		this->addSamplerDescriptor(binding);
	}
}

VulkanDescriptorSet::VulkanDescriptorSet( VulkanDescriptorSet&& other ) :
	vulkanDevice{other.vulkanDevice},
	framesInFlight{other.framesInFlight},
	currentFrame{other.currentFrame},
	descriptorSets{std::move(other.descriptorSets)},
	buffers{std::move(other.buffers)},
	textures{std::move(other.textures)}
{
}

void VulkanDescriptorSet::setCurrentFrame(uint32_t frame) noexcept
{
	assert(frame < this->framesInFlight && "Frame index over limit");
	this->currentFrame = frame;
}

void VulkanDescriptorSet::updateUniform(int32_t binding, void const* data) noexcept
{
	assert(this->buffers.count(binding) != 0U && "Buffer binding not found in descriptor set");
	this->buffers[binding][this->currentFrame]->writeToBuffer(data);
}

void VulkanDescriptorSet::updateUniformAll(int32_t binding, void const* data) noexcept
{
	assert(this->buffers.count(binding) != 0U && "Buffer binding not found in descriptor set");
	for (uint32_t frame = 0; frame < this->framesInFlight; frame++)
	{
		this->buffers[binding][frame]->writeToBuffer(data);
	}
}

void VulkanDescriptorSet::bindSet(VkCommandBuffer commandBuffer, VulkanPipeline const& pipeline, uint32_t setIndex) noexcept
{
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline.getPipelineLayout(),
		setIndex,
		1,
		&this->descriptorSets[this->currentFrame],
		0,
		nullptr
	);
}

void VulkanDescriptorSet::addBufferDescriptor(UniformBinding const& bindData) noexcept
{
	ui32 binding = bindData.binding;
	ui32 bufferSize = bindData.bufferSize;

	for (uint32_t frame = 0; frame < this->framesInFlight; frame++)
	{
		this->buffers[binding][frame] = std::make_unique<VulkanBuffer>(
			this->vulkanDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		this->buffers[binding][frame]->map();

		VkDescriptorBufferInfo bufferInfo = this->buffers[binding][frame]->descriptorInfo();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.dstSet = this->descriptorSets[frame];
		write.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(this->vulkanDevice.device(), 1, &write, 0, nullptr);
	}
}

void VulkanDescriptorSet::addSamplerDescriptor(SamplerBinding const& bindData) noexcept
{
	ui32				binding = bindData.binding;
	std::string const&	texturePath = bindData.texturePath;
	TextureType			textureType = bindData.textureType;

	this->textures[binding] = std::make_unique<VulkanTexture>(this->vulkanDevice, texturePath, textureType);

	for (uint32_t frame = 0; frame < this->framesInFlight; frame++)
	{
		VkDescriptorImageInfo imageInfo = this->textures[binding]->getDescriptorImageInfo();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.dstSet = this->descriptorSets[frame];
		write.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(this->vulkanDevice.device(), 1, &write, 0, nullptr);
	}
}

}	// namespace ve
