#pragma once

#include "VulkanDevice.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"

#include <vector>


namespace ve {

struct VulkanPipelineConfig
{
	VkPipelineVertexInputStateCreateInfo 	vertexInputInfo;
	VkPipelineViewportStateCreateInfo		viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo	inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo	rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo	multisampleInfo;
	VkPipelineColorBlendAttachmentState		colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo		colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo	depthStencilInfo;
	std::vector<VkDynamicState>				dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo		dynamicStateInfo;

	std::vector<VkVertexInputBindingDescription>	bindingVboConfig;
	std::vector<VkVertexInputAttributeDescription>	attributeVboConfig;
	std::vector<VkPipelineShaderStageCreateInfo>	shadersConfig;
};

class VulkanShader
{
	public:
		VulkanShader( void ) = delete;
		VulkanShader( VulkanDevice& device, VkShaderStageFlagBits shaderStageFlag, std::string const& shaderPath );
		~VulkanShader( void );
		VulkanShader( VulkanShader const& other ) = delete;
		VulkanShader( VulkanShader&& other );
		VulkanShader& operator=( VulkanShader const& other ) = delete;

		VkShaderModule			getModule( void ) const noexcept { return this->shaderModule; };
		VkShaderStageFlagBits	getStageFlag( void ) const noexcept { return this->shaderStageFlag; };

	private:
		void createModule(std::vector<char> const& fileContent);

		VulkanDevice&			vulkanDevice;
		VkShaderStageFlagBits	shaderStageFlag;
		std::string				shaderPath;
		VkShaderModule			shaderModule;
};

class VulkanPipeline
{
	public:
		VulkanPipeline( void ) = delete;
		VulkanPipeline(
			VulkanDevice& device,
			std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts,
			VkRenderPass renderPass,
			std::string const& vertexShaderFile,
			std::string const& fragmentShaderFile,
			MeshlayoutDescription const& meshLayout,
			bool hasCubemapsTexture,
			uint32_t sizePushConstants
		);
		~VulkanPipeline( void );
		VulkanPipeline( VulkanPipeline const& ) = delete;
		VulkanPipeline( VulkanPipeline&& );
		VulkanPipeline& operator=( VulkanPipeline const& ) = delete;

		VkPipelineLayout	getPipelineLayout( void ) const noexcept { return pipelineLayout; };
		void				bindPipeline(VkCommandBuffer commandBuffer ) const noexcept;
		void				updatePushConstants(VkCommandBuffer commandBuffer, void const* data ) const noexcept;

		static std::unique_ptr<VulkanPipeline> createPipeline(
			VulkanDevice& device,
			std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts,
			VkRenderPass renderPass,
			std::string const& vertexShaderFile,
			std::string const& fragmentShaderFile,
			MeshlayoutDescription const& meshLayout,
			bool hasCubemapsTexture = false,
			uint32_t sizePushConstants = 0U
		);

	private:
		void					setupPipelineLayout( std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts );
		void					setupPipeline( std::string const& vertexShaderFile, std::string const& fragmentShaderFile, MeshlayoutDescription const& meshLayout, bool hasCubemapsTexture, VkRenderPass renderPass );
		VulkanPipelineConfig	getPipelineConfig( std::vector<VulkanShader> const& shaders, MeshlayoutDescription const& meshLayout, bool hasCubemapsTexture ) const noexcept;

		VulkanDevice&		vulkanDevice;
		VkPipelineLayout	pipelineLayout;
		VkPipeline			pipeline;

		uint32_t			sizePushConstants;
};

}