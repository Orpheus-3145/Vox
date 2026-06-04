#version 450

layout(set = 1, binding = 0) uniform sampler2D terrainSampler;
layout(set = 1, binding = 1) uniform sampler2D undergroundSampler;
layout(set = 1, binding = 2) uniform samplerCube skyboxSampler;

layout(push_constant) uniform MeshData {
	mat4	modelMatrix;
	mat4	normalMatrix;
	uint	materialIndex;
	uint	lightIndex;
	uint	textureIndex;
} meshData;

layout(location = 2) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	if (meshData.textureIndex == 0)
		outColor = texture(terrainSampler, fragTextureUV);
	else
		outColor = texture(undergroundSampler, fragTextureUV);
}
