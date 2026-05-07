#pragma once
#include <cstdint>
#include <vector>

#include "Vectors.hpp"


namespace ve {

static_assert(sizeof(mat4) == 64 && "mat4 type size has to be 64B");
static_assert(sizeof(vec4) == 16 && "vec4 type size has to be 16B");

// [has to comply with std140]
class ViewProjectUniform
{
	public:
		ViewProjectUniform( void ) = delete;
		ViewProjectUniform( mat4 const& view, mat4 const& projection ) :
			view{view},
			projection{projection} {}

		void	updateView( mat4 const& view ) noexcept { this->view = view; }
		void	updateProjection( mat4 const& prj ) noexcept { this->projection = prj; }

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

	private:
		mat4 view;
		mat4 projection;
};

// Phong lighting model, a combination of the following:
// Ambient lighting: even when it is dark there is usually still some light somewhere in the world
// 		(the moon, a distant light) so objects are almost never completely dark. To simulate this
// 		we use an ambient lighting constant that always gives the object some color.
// Diffuse lighting: simulates the directional impact a light object has on an object. This is the
// 		most visually significant component of the lighting model. The more a part of an object
// 		faces the light source, the brighter it becomes.
// Specular lighting: simulates the bright spot of a light that appears on shiny objects. Specular
// 		highlights are more inclined to the color of the light than the color of the object.
// [has to comply with std140]
struct MaterialData
{
	vec4	ambientColor;			// range [0-1] - indirect light color (darker than diffuse)
	vec4	diffuseColor;			// range [0-1] - color of the mesh
	vec4	specularColor;			// range [0-1] - reflex of the light
	float	shininess;				// range [1.0-256.0] - low (2-8): opaque, high (64-256) shiny/metal
	float	opacity;				// alpha of diffuse
    int32_t	refractionIndex{1};		// [not used yet, but keep it for padding] range [1-2.42...] - index of refraction, 1.0: air, 1.33 h2o, 1.5 glass								
	int32_t	illuminationModel{1};	// [not used yet, but keep it for padding] range [0-10] - 0: no lighting only texture, 1: ambient + diffuse, 2: ambient + diffuse + specular
};

// [has to comply with std140]
struct LightData
{
	vec4	lightAmbientColor;
	vec4	lightColor;
	vec4	lightSpecularColor;
	vec4 	lightDir;
};

struct VkConstants
{
	uint32_t	nMaterials;
	uint32_t	nLights;
};

// [has to comply with std140]
class MaterialUniform
{
	public:
		void		updateMaterial( uint32_t index, MaterialData const& newMaterial ) noexcept;
		void		updateLight( uint32_t index, LightData const& newLight, mat4 const& viewMatrix ) noexcept;
		void		updateLightDir( uint32_t index, vec3 const& lightDir, mat4 const& viewMatrix ) noexcept;

		VkConstants	getConstants( void ) const noexcept { return VkConstants{MAX_MATERIALS, MAX_LIGHTS}; }
		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

		static constexpr uint32_t MAX_MATERIALS = 8;
		static constexpr uint32_t MAX_LIGHTS = 8;

	private:
		std::array<MaterialData, MAX_MATERIALS> materials;
		std::array<LightData, MAX_LIGHTS>		lights;
};

// NB [has to comply with std140]
class PushConstantsData
{
	public:
		PushConstantsData( void ) = delete;
		PushConstantsData( mat4 const& modelMatrix, mat4 const& normalMatrix ) :
			modelMatrix{modelMatrix},
			normalMatrix{normalMatrix} {};

		void	setModelMatrix( mat4 const& modelMatrix ) noexcept { this->modelMatrix = modelMatrix; }
		void	setNormalMatrix( mat4 const& normalMatrix ) noexcept { this->normalMatrix = normalMatrix; }
		void	setMaterialIndex( uint32_t index ) noexcept;
		void	setLightIndex( uint32_t index ) noexcept;
		void	setTextureIndex( uint32_t index ) noexcept { this->textureIndex = index; }

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

	private:
		mat4		modelMatrix;
		mat4		normalMatrix;
		uint32_t	materialIndex{0U};
		uint32_t	lightIndex{0U};
		uint32_t	textureIndex{0U};
};

}	// namespace ve
