#version 450

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
	mat4	orthographic;
}	ubo;

layout(push_constant) uniform MeshData {
	mat4	modelMatrix;
	mat4	normalMatrix;

	vec4	ambientClr;
	vec4	diffuseClr;
	vec4	specularClr;
	float	shininess;
	float	opacity;
} meshData;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureUV;

layout(location = 2) out vec2 fragTextureUV;


void main()
{
	fragTextureUV = textureUV;

	gl_Position = ubo.projection * ubo.view * meshData.modelMatrix * vec4(position, 1.0f);
}
