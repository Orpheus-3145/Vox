#include <cassert>

#include "VulkanUniform.hpp"


namespace ve {

void	MeshUniform::updateModelMatrix( uint32_t index, mat4 const& modelMatrix ) noexcept
{
	assert(index < drawingDataLimits.models && "model matrix index out of bounds");

	this->models[index] = modelMatrix;
}

void	MeshUniform::updateNormalMatrix( uint32_t index, mat4 const& normalMatrix ) noexcept
{
	assert(index < drawingDataLimits.models && "normal matrix index out of bounds");

	this->normals[index] = normalMatrix;
}

void	MeshUniform::updateMaterial( uint32_t index, MaterialData const& newMaterial ) noexcept
{
	assert(index < drawingDataLimits.materials && "material index out of bounds");

	this->materials[index] = newMaterial;
}

void	MeshUniform::updateLight( uint32_t index, LightData const& newLight, mat4 const& viewMatrix ) noexcept
{
	assert(index < drawingDataLimits.lights && "light source index out of bounds");

	this->lights[index] = newLight;
	this->updateLightDir(index, vec3{newLight.lightDir.x, newLight.lightDir.y, newLight.lightDir.z}, viewMatrix);
}

void	MeshUniform::updateLightDir( uint32_t index, vec3 const& newDir, mat4 const& viewMatrix ) noexcept
{
	assert(index < drawingDataLimits.lights && "light source index out of bounds");

	// light direction is in view space to avoid passing camera position to shaders
	vec4 reverseLightDir = vec4{newDir * -1, 0.0f};
	vec4 viewLightDir = (viewMatrix * reverseLightDir).normalize();

	this->lights[index].lightDir = viewLightDir;
}


void	TextUniform::updateColor( uint32_t index, vec4 const& color ) noexcept
{
	assert(index < drawingDataLimits.fontColor && "text color index out of bounds");

	this->color[index] = color;
}

}	// namespace ve
