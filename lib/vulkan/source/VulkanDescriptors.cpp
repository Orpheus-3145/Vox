#include "VulkanDescriptors.hpp"
#include "VulkanTexture.hpp"

#include <cassert>


namespace ve {

VulkanBindingSet&	VulkanBindingSet::addBinding( uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count )
{
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = type;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stage;

	this->bindings.push_back(layoutBinding);
	return *this;
}


VulkanDescriptorSetFactory::~VulkanDescriptorSetFactory( void )
{
	if (this->descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, nullptr);
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addPoolSize(VkDescriptorType type, uint32_t count)
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = type;
	poolSize.descriptorCount = count;

	this->poolSizes.push_back(poolSize);
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
	for ( VkDescriptorPoolSize& poolConfig : this->poolSizes )
		poolConfig.descriptorCount *= this->framesInFlight;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = this->poolSizes.size();
	poolInfo.pPoolSizes = this->poolSizes.data();
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
	if (this->descriptorPool != VK_NULL_HANDLE)
		vkResetDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, 0);
	return *this;
}

std::unique_ptr<VulkanDescriptorSet> VulkanDescriptorSetFactory::createDescriptorSet( VulkanBindingSet const& bindings ) const
{
	return std::make_unique<VulkanDescriptorSet>(
		this->vulkanDevice,
		this->framesInFlight,
		this->descriptorPool,
		bindings
	);
}


VulkanDescriptorSet::VulkanDescriptorSet(
	VulkanDevice& vulkanDevice,
	uint32_t framesInFlight,
	VkDescriptorPool descriptorPool,
	VulkanBindingSet const& bindings
) :
	vulkanDevice{vulkanDevice},
	framesInFlight{framesInFlight},
	descriptorSetLayout{VK_NULL_HANDLE}
{
	VkDescriptorSetLayoutCreateInfo	descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = bindings.getbindings().size();
	descriptorSetLayoutInfo.pBindings = bindings.getbindings().data();

	if (vkCreateDescriptorSetLayout(
		this->vulkanDevice.device(),
		&descriptorSetLayoutInfo,
		nullptr,
		&this->descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout");
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &this->descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	this->descriptorSets.resize(this->framesInFlight);
	for (uint32_t frame = 0U; frame < this->framesInFlight; frame++)
	{
		if (vkAllocateDescriptorSets(this->vulkanDevice.device(), &allocInfo, &this->descriptorSets[frame]) != VK_SUCCESS)
		{
			vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), this->descriptorSetLayout, nullptr);
			throw std::runtime_error("failed to create descriptor set");
		}
	}

	for (VkDescriptorSetLayoutBinding const& binding : bindings.getbindings())
	{
		if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			buffers[binding.binding] = std::vector<std::unique_ptr<VulkanBuffer>>(this->framesInFlight);
		}
		else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			textures[binding.binding] = nullptr;
		}
	}
}

VulkanDescriptorSet::~VulkanDescriptorSet( void )
{
	if (this->descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), this->descriptorSetLayout, nullptr);
}

VulkanDescriptorSet::VulkanDescriptorSet( VulkanDescriptorSet&& other ) :
	vulkanDevice{other.vulkanDevice},
	framesInFlight{other.framesInFlight},
	descriptorSetLayout{other.descriptorSetLayout},
	currentFrame{other.currentFrame},
	descriptorSets{std::move(other.descriptorSets)},
	buffers{std::move(other.buffers)},
	textures{std::move(other.textures)}
{
	other.descriptorSetLayout = VK_NULL_HANDLE;
}

void VulkanDescriptorSet::setCurrentFrame(uint32_t frame) noexcept
{
	assert(frame < this->framesInFlight && "Frame index over limit");
	this->currentFrame = frame;
}

void VulkanDescriptorSet::updateUbo(int32_t binding, void const* data) noexcept
{
	assert(this->buffers.count(binding) != 0U && "Buffer binding not found in descriptor set");
	this->buffers[binding][this->currentFrame]->writeToBuffer(data);
}

void VulkanDescriptorSet::updateUboAll(int32_t binding, void const* data) noexcept
{
	assert(this->buffers.count(binding) != 0U && "Buffer binding not found in descriptor set");
	for (uint32_t frame = 0; frame < this->framesInFlight; frame++)
		this->buffers[binding][frame]->writeToBuffer(data);
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

void VulkanDescriptorSet::addBufferDescriptor(uint32_t binding, uint32_t bufferSize) noexcept
{
	assert(this->buffers.count(binding) != 0U && "Buffer binding not found in descriptor set");

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
		// this->buffers[binding][frame]->writeToBuffer(data);

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

void VulkanDescriptorSet::addSamplerDescriptor(uint32_t binding, const std::string& texturePath, TextureType type) noexcept
{
	assert(this->textures.count(binding) != 0U && "Sampler binding not found in descriptor set");

	this->textures[binding] = std::make_unique<VulkanTexture>(this->vulkanDevice, texturePath, type);

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
