#pragma once

#include <memory>
#include <map>
#include <string>
#include <atomic>

#include "Vectors.hpp"
#include "VulkanModel.hpp"


namespace ve {

struct Material
{
	std::string	name;
	vec4	ambientClr;
	vec4	diffuseClr;
	vec4	specularClr;
	int32_t	shininess;
	float	opacity;
	int32_t	refractionIndex;
	int32_t	illuminationModel;
	bool	smoothShading;
};

struct ObjComponent
{
	std::vector<IndexVector>	faceIndices;
	std::vector<IndexVector>	textureIndices;
	std::vector<IndexVector>	normalIndices;
	std::string							matName;
};

struct ObjInfo
{
	std::string						name;
	std::string						mtlFile;
	std::vector<vec3>				vertices;
	std::vector<vec2>				textureCoords;
	std::vector<vec3>				normals;
	std::vector<vec3>				colors;
	std::vector<ObjComponent>		components;
	std::map<std::string, Material>	materials;
};

class VulkanObject
{
	public:
		VulkanObject() : id(currentID.fetch_add(1, std::memory_order_relaxed)) {};
		VulkanObject(const VulkanObject& other) = delete;
		VulkanObject(VulkanObject&& other) = default;
		VulkanObject& operator=(const VulkanObject& other) = delete;
		VulkanObject& operator=(VulkanObject&& other) = delete;
		~VulkanObject() = default;

		void	rotate( vec3 const& axis, float angle ) noexcept;
		void	translate( vec3 const& translation ) noexcept;
		void	scale( vec3 const& scale ) noexcept;
		void	scale( float scale ) noexcept;
		
		void	bindBuffer(VkCommandBuffer commandBuffer) const noexcept;
		void	draw(VkCommandBuffer commandBuffer) const noexcept;
		
		void							setModel(std::shared_ptr<VulkanModel> newModel) noexcept { this->model = newModel; };  // NB add createModel (that takes the input for the VulkanModel constructor)
		std::shared_ptr<VulkanModel>	getModel() const noexcept;
		uint32_t						getID() const noexcept { return this->id; }
		MeshLayoutDescription			getModelLayout() const noexcept;

		mat4							getModelMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalViewMatrix(const mat4& viewNoTranslation, bool columnMajor = false) const noexcept;

	private:
		uint32_t						id;
		std::shared_ptr<VulkanModel>	model{nullptr};
		
		vec3	_translation{0.0f};
		vec3	_scale{1.0f, 1.0f, 1.0f};
		quat	_rotation{};

		bool	transformationApplied{false};
		bool	uniformScale{true};

		static inline std::atomic<uint32_t> currentID;
};

} // namespace ve
