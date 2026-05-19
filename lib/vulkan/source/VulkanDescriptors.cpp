#include "VulkanDescriptors.hpp"
#include "VulkanTexture.hpp"

#include <cassert>


namespace ve {

BindInfo::~BindInfo( void ) {}


uint32_t VulkanBindingSet::ID_INSTANCE = 0U;

VulkanBindingSet&	VulkanBindingSet::addBufferBinding( uint32_t binding, VkShaderStageFlags stage, uint32_t bufferSize, BufferType bufferType )
{
	if (bufferType != BUFFER_UNIFORM and bufferType != BUFFER_STORAGE)
	{
		throw std::runtime_error("buffer type can be only uniform or storage");
	}

	VkDescriptorType vkType = (bufferType == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	this->addBinding(binding, vkType, stage, 1);

	this->bindings.emplace_back(new UniformBindInfo(this->vkBindings.back(), std::vector<uint32_t>{bufferSize}, bufferType));

	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addSamplerBinding( uint32_t binding, VkShaderStageFlags stage, std::string const& texturePath, TextureType textureType )
{
	this->addBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, 1);
	this->bindings.emplace_back(new SamplerBindInfo(this->vkBindings.back(), std::vector<std::string>{texturePath}, std::vector<TextureType>{textureType}));

	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addBufferArrayBinding( uint32_t binding, VkShaderStageFlags stage, std::vector<uint32_t> const& sizes, BufferType bufferType )
{
	if (bufferType != BUFFER_UNIFORM and bufferType != BUFFER_STORAGE)
	{
		throw std::runtime_error("buffer type can be only uniform or storage");
	}

	uint32_t nBuffers = sizes.size();
	assert(nBuffers > 0U and "no buffer size provided");

	VkDescriptorType vkType = (bufferType == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	this->addBinding(binding, vkType, stage, nBuffers);

	this->bindings.emplace_back(new UniformBindInfo(this->vkBindings.back(), sizes, bufferType));

	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addSamplerArrayBinding( uint32_t binding, VkShaderStageFlags stage, std::vector<std::string> const& texturePaths, std::vector<TextureType> types )
{
	if (texturePaths.size() != types.size())
	{
		throw std::runtime_error("texture paths and texture types differ in quantity");
	}

	uint32_t nSamplers = texturePaths.size();
	assert(nSamplers > 0U and "no texture info provided");

	this->addBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stage, nSamplers);
	this->bindings.emplace_back(new SamplerBindInfo(this->vkBindings.back(), texturePaths, types));

	return *this;
}

void VulkanBindingSet::addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count)
{
	for (VkDescriptorSetLayoutBinding const& bindingInfo : this->vkBindings)
	{
		if (bindingInfo.binding == binding)
		{
			throw std::runtime_error("binding for descriptor already set");
		}
	}
	VkDescriptorSetLayoutBinding newBinding{};
	newBinding.binding = binding;
	newBinding.descriptorType = type;
	newBinding.descriptorCount = count;
	newBinding.stageFlags = stage;

	this->vkBindings.emplace_back(newBinding);
}


VulkanDescriptorSetFactory::~VulkanDescriptorSetFactory( void ) noexcept
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

VulkanDescriptorSetFactory::VulkanDescriptorSetFactory( VulkanDescriptorSetFactory& other) noexcept :
	vulkanDevice{other.vulkanDevice}
{
	if (this != &other)
	{
		this->poolFlags = other.poolFlags;
		this->maxSets = other.maxSets;
		this->countTypes = std::move(other.countTypes);
		this->descriptorPool = other.descriptorPool;
		this->existingLayouts = std::move(other.existingLayouts);
		this->descriptorSetlayouts = std::move(other.descriptorSetlayouts);

		other.descriptorPool = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addPoolSize(VkDescriptorType type, uint32_t count)
{
	if (this->countTypes.count(type) == 0)
	{
		throw std::runtime_error("only descriptor type supported: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");
	}

	this->countTypes[type] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addBufferPoolSize(uint32_t count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addSsboPoolSize(uint32_t count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] += count;
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

VulkanDescriptorSetFactory& VulkanDescriptorSetFactory::createPool( void )
{
	std::vector<VkDescriptorPoolSize> poolSizes{};
	if (this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]
		});
	}
	if (this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER]
		});
	}
	if (this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
		});
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = this->maxSets;
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

	this->poolFlags = 0U;
	this->maxSets = 0U;
	this->countTypes.clear();
	return *this;
}

std::unique_ptr<VulkanDescriptorSet> VulkanDescriptorSetFactory::createDescriptorSet( VulkanBindingSet const& bindings )
{
	if (this->descriptorPool == VK_NULL_HANDLE)
	{
		throw std::runtime_error("pool not created");
	}
	else if(this->maxSets == 0U)
	{
		throw std::runtime_error("not enough sets left from the pool");
	}
	this->maxSets--;

	uint32_t countUBOs = 0U, countSSBOs = 0U, countSamplers = 0U;
	for(std::unique_ptr<BindInfo> const& bindData : bindings.getBindingData())
	{
		if (dynamic_cast<UniformBindInfo*>(bindData.get()))
		{
			UniformBindInfo* uniformBinding	= dynamic_cast<UniformBindInfo*>(bindData.get());
			(uniformBinding->getBufferType() == BUFFER_UNIFORM) ? countUBOs += uniformBinding->getNitems() : countSSBOs += uniformBinding->getNitems();
	
			for (uint32_t size : uniformBinding->getBufferSizes())
			{
				if (size > this->vulkanDevice.getMaxSizeUniformBuffer())
				{
					throw std::runtime_error("uniform buffer size exceeds device limit");
				}
			}
		}
		else if (dynamic_cast<SamplerBindInfo*>(bindData.get()))
		{
			countSamplers += bindData->getNitems();
		}
	}

	if ((countUBOs + countSSBOs + countSamplers) == 0U)
	{
		throw std::runtime_error("no binding set");
	}

	if(this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] < countUBOs)
	{
		throw std::runtime_error("not enough uniform buffer descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] -= countUBOs;

	if(this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] < countSSBOs)
	{
		throw std::runtime_error("not enough storage buffer descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] -= countSSBOs;

	if(this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] < countSamplers)
	{
		throw std::runtime_error("not enough sampler descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] -= countSamplers;

	// create a new descLayout if none is found linked to this binding
	if (this->existingLayouts.count(bindings.getId()) == 0U)
	{
		this->addNewLayout(bindings.getVkBindingData(), bindings.getVkBindingDataSize(), bindings.getId());
	}

	return std::make_unique<VulkanDescriptorSet>(
		this->vulkanDevice,
		this->existingLayouts[bindings.getId()],
		this->descriptorPool,
		bindings
	);
}

void VulkanDescriptorSetFactory::addNewLayout( VkDescriptorSetLayoutBinding const* bindingData, uint32_t size, uint32_t idBinding )
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
	VkDescriptorSetLayout	descriptorSetLayout,
	VkDescriptorPool		descriptorPool,
	VulkanBindingSet const&	bindings
)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	if (vkAllocateDescriptorSets(vulkanDevice.device(), &allocInfo, &this->descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set");
	}

	for(std::unique_ptr<BindInfo> const& bindData : bindings.getBindingData())
	{
		if (dynamic_cast<UniformBindInfo*>(bindData.get()))
		{
			UniformBindInfo* uniformBinding	= dynamic_cast<UniformBindInfo*>(bindData.get());
			this->descriptors[bindData->getBinding()] = std::make_unique<VulkanBufferDescriptor>(*uniformBinding, vulkanDevice, this->descriptorSet);
		}
		else if (dynamic_cast<SamplerBindInfo*>(bindData.get()))
		{
			SamplerBindInfo* samplerBinding	= dynamic_cast<SamplerBindInfo*>(bindData.get());
			this->descriptors[bindData->getBinding()] = std::make_unique<VulkanSamplerDescriptor>(*samplerBinding, vulkanDevice, this->descriptorSet);
		}
	}
}

void VulkanDescriptorSet::updateDescriptor(int32_t binding, void const* data, uint32_t index) noexcept
{
	assert(this->descriptors.count(binding) != 0U && "Buffer binding not found in descriptor set");

	this->descriptors[binding]->update(data, index);
}

void VulkanDescriptorSet::bindSet(VkCommandBuffer commandBuffer, VulkanPipeline const& pipeline, uint32_t setIndex) noexcept
{
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline.getPipelineLayout(),
		setIndex,
		1,
		&this->descriptorSet,
		0,
		nullptr
	);
}


VulkanDescriptor::~VulkanDescriptor( void ) {}


VulkanBufferDescriptor::VulkanBufferDescriptor(UniformBindInfo const& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet)
	: VulkanDescriptor(binding)
{
	std::vector<uint32_t> const&	bufferSizes = binding.getBufferSizes();
	BufferType						bufferType = binding.getBufferType();
	uint32_t 						nBuffers = binding.getNitems();

	std::vector<VkDescriptorBufferInfo> bufferInfo(nBuffers);
	this->buffers.resize(nBuffers);

	for(uint32_t i = 0U; i < nBuffers; i++)
	{
		this->buffers[i] = std::make_unique<VulkanBuffer>(
			vulkanDevice,
			bufferSizes[i],
			1,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			bufferType
		);
		this->buffers[i]->map();

		bufferInfo[i] = this->buffers[i]->descriptorBufferInfo();
	}

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = this->binding;
	write.descriptorType = (binding.getBufferType() == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = nBuffers;
	write.dstSet = descriptorSet;
	write.pBufferInfo = bufferInfo.data();

	vkUpdateDescriptorSets(vulkanDevice.device(), 1, &write, 0, nullptr);
}

void VulkanBufferDescriptor::update( void const* data, uint32_t index ) noexcept
{
	assert(index < this->buffers.size() && "Buffer binding not found in descriptor set");

	this->buffers[index]->writeToBuffer(data);
}


VulkanSamplerDescriptor::VulkanSamplerDescriptor(SamplerBindInfo const& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet)
	: VulkanDescriptor(binding)
{
	std::vector<std::string> const&	texturePaths = binding.getTexturePaths();
	std::vector<TextureType>		textureTypes = binding.getTextureTypes();
	uint32_t 						nTextures = binding.getNitems();

	std::vector<VkDescriptorImageInfo> textureInfo(nTextures);
	this->textures.resize(nTextures);

	for (uint32_t i = 0; i < nTextures; i++)
	{
		this->textures[i] = std::make_unique<VulkanTexture>(vulkanDevice, texturePaths[i], textureTypes[i]);
		textureInfo[i] = this->textures[i]->getDescriptorImageInfo();
	}
		
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = this->binding;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = nTextures;
	write.dstSet = descriptorSet;
	write.pImageInfo = textureInfo.data();

	vkUpdateDescriptorSets(vulkanDevice.device(), 1, &write, 0, nullptr);
}

void VulkanSamplerDescriptor::update( void const* data, uint32_t index ) noexcept
{
	(void) data;
	(void) index;
}

}	// namespace ve
