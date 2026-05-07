#include "VulkanUniform.hpp"
#include <cassert>


namespace ve {

void	MaterialUniform::updateMaterial( uint32_t index, MaterialData const& newMaterial ) noexcept
{
	assert(index < MaterialUniform::MAX_MATERIALS && "material index out of bounds");

	this->materials[index] = newMaterial;
}

void	MaterialUniform::updateLight( uint32_t index, LightData const& newLight, mat4 const& viewMatrix ) noexcept
{
	assert(index < MaterialUniform::MAX_LIGHTS && "light source index out of bounds");

	this->lights[index] = newLight;
	this->updateLightDir(index, vec3(newLight.lightDir), viewMatrix);
}

void	MaterialUniform::updateLightDir( uint32_t index, vec3 const& lightDir, mat4 const& viewMatrix ) noexcept
{
	assert(index < MaterialUniform::MAX_LIGHTS && "light source index out of bounds");

	vec4 lightDir4 = vec4{lightDir * -1, 0.0f};
	lightDir4 = viewMatrix * lightDir4;
	lightDir4.normalize();
	this->lights[index].lightDir = lightDir4;
}


void	PushConstantsData::setMaterialIndex( uint32_t index ) noexcept
{
	assert(index < MaterialUniform::MAX_MATERIALS && "material index out of bounds");

	this->materialIndex = index;
}

void	PushConstantsData::setLightIndex( uint32_t index ) noexcept
{
	assert(index < MaterialUniform::MAX_LIGHTS && "light source index out of bounds");

	this->lightIndex = index;
}

}	// namespace ve
