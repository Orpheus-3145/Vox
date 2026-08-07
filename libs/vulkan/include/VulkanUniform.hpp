#pragma once

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
		ViewProjectUniform( mat4 const& view, mat4 const& projection, mat4 const& orthographic ) :
			view{view},
			projection{projection},
			orthographic{orthographic} {}

		void	updateView( mat4 const& view ) noexcept { this->view = view; }
		void	updateProjection( mat4 const& prj ) noexcept { this->projection = prj; }
		void	updateOrthographic( mat4 const& ortho ) noexcept { this->orthographic = ortho; }

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

	private:
		mat4	view;
		mat4	projection;
		mat4	orthographic;
};
// check this: https://chatgpt.com/s/t_6a0c79f4930881919ea36656e039782a
static_assert((sizeof(ViewProjectUniform) % 16 == 0) && "type not ...");

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
	uint32_t	models{0U};
	uint32_t	materials{0U};
	uint32_t	lights{0U};
	uint32_t	fontColor{0U};
	uint32_t	textures{0U};
};

inline constexpr VkConstants drawingDataLimits{8U, 8U, 1U, 2U, 2U};		// NB this values depends on the actual number of entities used, better move it to vox

// [has to comply with std140]
class MeshUniform
{
	public:
		void	updateModelMatrix( uint32_t index, mat4 const& modelMatrix ) noexcept;
		void	updateNormalMatrix( uint32_t index, mat4 const& normalMatrix ) noexcept;
		void	updateMaterial( uint32_t index, MaterialData const& newMaterial ) noexcept;
		void	updateLight( uint32_t index, LightData const& newLight, mat4 const& viewMatrix ) noexcept;
		void	updateLightDir( uint32_t index, vec3 const& newDir, mat4 const& viewMatrix ) noexcept;

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

	private:
		mat4 			models[drawingDataLimits.models];
		mat4 			normals[drawingDataLimits.models];
		MaterialData 	materials[drawingDataLimits.materials];
		LightData 		lights[drawingDataLimits.lights];
};


// [has to comply with std140]
class TextUniform
{
	public:
		void	updateColor( uint32_t index, vec4 const& color ) noexcept;

		const void*	getData( void ) const noexcept { return static_cast<const void*>(this); }

	private:
		vec4	color[drawingDataLimits.fontColor];
};

class PushConstantsData {};

}	// namespace ve
