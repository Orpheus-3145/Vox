#include "VulkanPipeline.hpp"
#include "VulkanUtils.hpp"

#include <cassert>


namespace ve {

VulkanShader::VulkanShader( VulkanDevice& device, VkShaderStageFlagBits	shaderStageFlag, std::string const& shaderPath) : 
	vulkanDevice{device}, shaderStageFlag{shaderStageFlag}, shaderPath{shaderPath}, shaderModule{VK_NULL_HANDLE}
{
	std::vector<char> content = readFile(shaderPath);
	this->createModule(content);
}

VulkanShader::VulkanShader( VulkanShader&& other ) :
	vulkanDevice{other.vulkanDevice}, shaderStageFlag{other.shaderStageFlag}, shaderPath{other.shaderPath}, shaderModule{other.shaderModule}
{
	other.shaderModule = VK_NULL_HANDLE;
}

VulkanShader::~VulkanShader( void )
{
	if (this->shaderModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(this->vulkanDevice.device(), this->shaderModule, nullptr);
}

void VulkanShader::createModule(std::vector<char> const& fileContent)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileContent.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(fileContent.data());

	if (vkCreateShaderModule(
			vulkanDevice.device(),
			&createInfo,
			nullptr,
			&this->shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}
}


std::unique_ptr<VulkanPipeline> VulkanPipeline::createPipeline(
	VulkanDevice& device,
	std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts,
	VkRenderPass renderPass,
	std::string const& vertexShaderFile,
	std::string const& fragmentShaderFile,
	MeshlayoutDescription const& meshLayout,
	bool hasCubemapsTexture,
	uint32_t sizePushConstants
)
{
	return std::make_unique<VulkanPipeline>(
		device,
		descriptorSetLayouts,
		renderPass,
		vertexShaderFile,
		fragmentShaderFile,
		meshLayout,
		hasCubemapsTexture,
		sizePushConstants
	);
}

VulkanPipeline::VulkanPipeline(
		VulkanDevice& device,
		std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts,
		VkRenderPass renderPass,
		std::string const& vertexShaderFile,
		std::string const& fragmentShaderFile,
		MeshlayoutDescription const& meshLayout,
		bool hasCubemapsTexture,
		uint32_t sizePushConstants
	) :
	vulkanDevice{device}, sizePushConstants{sizePushConstants}
{
	assert( this->sizePushConstants <= this->vulkanDevice.getMaxPushConstantsSize() && "Push constants size exceeds limit");

	this->setupPipelineLayout(descriptorSetLayouts);
	this->setupPipeline(vertexShaderFile, fragmentShaderFile, meshLayout, hasCubemapsTexture, renderPass);
}

VulkanPipeline::~VulkanPipeline()
{
	if (this->pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(this->vulkanDevice.device(), this->pipeline, nullptr);
	if (this->pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(this->vulkanDevice.device(), this->pipelineLayout, nullptr);
}

VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) :
	vulkanDevice{other.vulkanDevice}, pipelineLayout{other.pipelineLayout}, pipeline{other.pipeline}
{
	other.pipelineLayout = VK_NULL_HANDLE;
	other.pipeline = VK_NULL_HANDLE;
}

void VulkanPipeline::bindPipeline(VkCommandBuffer commandBuffer) const noexcept
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
}

void VulkanPipeline::updatePushConstants(VkCommandBuffer commandBuffer, void const* data ) const noexcept
{
	assert(data && "Data source null");

	vkCmdPushConstants(
		commandBuffer,
		this->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		this->sizePushConstants,
		data
	);
}

void VulkanPipeline::setupPipelineLayout(std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo	pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	VkPushConstantRange pushRange{};
	if (this->sizePushConstants > 0U)
	{
		pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.offset = 0;
		pushRange.size = this->sizePushConstants;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
	}

	if (vkCreatePipelineLayout(this->vulkanDevice.device(), &pipelineLayoutInfo, nullptr, &this->pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void VulkanPipeline::setupPipeline(std::string const& vertexShaderFile, std::string const& fragmentShaderFile, MeshlayoutDescription const& meshLayout, bool hasCubemapsTexture, VkRenderPass renderPass)
{
	assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	std::vector<ve::VulkanShader> shaders;
	shaders.emplace_back(this->vulkanDevice, VK_SHADER_STAGE_VERTEX_BIT, vertexShaderFile);
	shaders.emplace_back(this->vulkanDevice, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFile);

	VulkanPipelineConfig pipelineConfig = this->getPipelineConfig(shaders, meshLayout, hasCubemapsTexture);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = pipelineConfig.shadersConfig.size();
	pipelineInfo.pStages = pipelineConfig.shadersConfig.data();
	pipelineInfo.pVertexInputState = &pipelineConfig.vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &pipelineConfig.inputAssemblyInfo;
	pipelineInfo.pViewportState = &pipelineConfig.viewportInfo;
	pipelineInfo.pRasterizationState = &pipelineConfig.rasterizationInfo;
	pipelineInfo.pMultisampleState = &pipelineConfig.multisampleInfo;
	pipelineInfo.pColorBlendState = &pipelineConfig.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &pipelineConfig.depthStencilInfo;
	pipelineInfo.pDynamicState = &pipelineConfig.dynamicStateInfo;
	pipelineInfo.layout = this->pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(
		this->vulkanDevice.device(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&this->pipeline) != VK_SUCCESS)
	{
		vkDestroyPipelineLayout(this->vulkanDevice.device(), this->pipelineLayout, nullptr);
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

VulkanPipelineConfig VulkanPipeline::getPipelineConfig( std::vector<VulkanShader> const& shaders, MeshlayoutDescription const& meshLayout, bool hasCubemapsTexture ) const noexcept
{
	assert(this->pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	VulkanPipelineConfig configInfo{};
	configInfo.bindingVboConfig = meshLayout.bindingConfig;
	configInfo.attributeVboConfig = meshLayout.attributeConfig;
	configInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	configInfo.vertexInputInfo.pNext = nullptr;
	configInfo.vertexInputInfo.flags = 0U;
	configInfo.vertexInputInfo.vertexBindingDescriptionCount = configInfo.bindingVboConfig.size();
	configInfo.vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingVboConfig.data();
	configInfo.vertexInputInfo.vertexAttributeDescriptionCount = configInfo.attributeVboConfig.size();
	configInfo.vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeVboConfig.data();

	configInfo.shadersConfig.resize(shaders.size());
	for (uint32_t i = 0U; i < shaders.size(); i++)
	{
		configInfo.shadersConfig[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		configInfo.shadersConfig[i].pNext = nullptr;
		configInfo.shadersConfig[i].flags = 0;
		configInfo.shadersConfig[i].stage = shaders[i].getStageFlag();
		configInfo.shadersConfig[i].module = shaders[i].getModule();
		configInfo.shadersConfig[i].pName = "main";
		configInfo.shadersConfig[i].pSpecializationInfo = nullptr;
	}

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;
	configInfo.viewportInfo.pViewports = nullptr;
	configInfo.viewportInfo.scissorCount = 1;
	configInfo.viewportInfo.pScissors = nullptr;

	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	configInfo.rasterizationInfo.lineWidth = 1.0f;
	configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
	configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
	configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
	configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	configInfo.multisampleInfo.minSampleShading = 1.0f;
	configInfo.multisampleInfo.pSampleMask = nullptr;
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

	configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
	configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	configInfo.colorBlendInfo.attachmentCount = 1;
	configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.minDepthBounds = 0.0f;
	configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;

	if (hasCubemapsTexture == TEXTURE_CUBEMAP)
	{
		configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	}
	else
	{
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	}

	configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateInfo.flags = 0;
	configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();

	return configInfo;
}

} // namespace ve