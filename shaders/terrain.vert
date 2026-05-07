#version 450

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
}	matrixUbo;

layout(push_constant) uniform MeshData {
	mat4	modelMatrix;
	mat4	normalMatrix;
	int		materialIndex;
	int		lightIndex;
} meshData;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureUV;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTextureUV;


void main()
{
	vec4 worldPos = matrixUbo.view * meshData.modelMatrix * vec4(position, 1.0f);
	fragNormal = normalize(mat3(meshData.normalMatrix) * normal);
	fragTextureUV = textureUV;
	fragPos = worldPos.xyz;

	gl_Position = matrixUbo.projection * worldPos;
}
