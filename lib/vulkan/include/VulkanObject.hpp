#pragma once

#include "Vectors.hpp"
#include "VulkanModel.hpp"

#include <memory>
#include <map>
#include <string>
#include <unordered_map>

namespace ve {

struct ImageInfo
{
	const unsigned char*	imageData;
	int32_t					width;
	int32_t					height;
	int32_t					channels;
};

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

struct MeshMaterial
{
	vec4	ambientColor;		// range [0-1] - indirect light color (darker than diffuse)
	vec4	diffuseColor;		// range [0-1] - color of the mesh
	vec4	specularColor;		// range [0-1] - reflex of the light
	float	shininess;			// range [1.0-256.0] - low (2-8): opaque, high (64-256) shiny/metal
	float	opacity;			// alpha of diffuse
	int32_t	refractionIndex;	// range [1-2.42...] - index of refraction, 1.0: air, 1.33 h2o, 1.5 glass
	int32_t	illuminationModel;	// range [0-10] - 0: no lighting only texture, 1: ambient + diffuse, 2: ambient + diffuse + specular
};

static_assert(sizeof(MeshMaterial) == 64);

struct ObjComponent
{
	std::vector<std::vector<uint32_t>>	faceIndices;
	std::vector<std::vector<uint32_t>>	textureIndices;
	std::vector<std::vector<uint32_t>>	normalIndices;
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
		VulkanObject() : id(currentID++) {};
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
		
		void							setModel(std::shared_ptr<VulkanModel> newModel) noexcept { this->model = newModel; };
		std::shared_ptr<VulkanModel>	getModel() const noexcept;
		void							setMaterial(MeshMaterial const& material) noexcept { this->materialData = material; };
		MeshMaterial const&				getMaterial() const noexcept { return this->materialData; };
		uint32_t						getID() const noexcept { return this->id; }
		MeshlayoutDescription			getVboLayout() const noexcept;

		mat4							getModelMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalViewMatrix(const mat4& viewNoTranslation, bool columnMajor = false) const noexcept;

	private:
		uint32_t						id;
		std::shared_ptr<VulkanModel>	model{nullptr};
		MeshMaterial					materialData{};
		
		vec3	_translation{0.0f};
		vec3	_scale{1.0f, 1.0f, 1.0f};
		quat	_rotation{};

		bool	transformationApplied{false};
		bool	uniformScale{true};

		static uint32_t		currentID;
};

std::ostream&	operator<<(std::ostream& os, const ObjInfo& obj);
ImageInfo		loadImage(const std::string& imagePath);
std::vector<ObjInfo>	parseOBJFile(const std::string& objFilePath);

} // namespace ve
