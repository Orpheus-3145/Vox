#version 460

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
	mat4	orthographic;
}	matrixUbo;

// default values if not overridden
layout(constant_id = 0) const uint MAX_OBJS = 8;
layout(constant_id = 1) const uint MAX_MATERIALS = 8;
layout(constant_id = 2) const uint MAX_LIGHTS = 8;
layout(constant_id = 3) const uint MAX_FONTS = 8;
layout(constant_id = 4) const uint MAX_TEXTURES = 2;

struct MaterialData {
	vec4	ambientColor;		// currently not used, since there's the color of the texture
	vec4	diffuseColor;		// currently not used, since there's the color of the texture
	vec4	specularColor;
	float	shininess;
	float	opacity;
    int		refractionIndex;
    int		illuminationModel;
};

struct LightData {
	vec4	lightAmbientColor;
	vec4	lightColor;
	vec4	lightSpecularColor;
	vec4 	lightDir;
};

layout(set = 0, binding = 1) uniform MeshData {
	mat4			modelMatrix[MAX_OBJS];
	mat4			normalMatrix[MAX_OBJS];
	MaterialData	materialIndex[MAX_MATERIALS];
	LightData		lightIndex[MAX_LIGHTS];
} meshData;

layout(push_constant) uniform DescriptorIndexes {
	uint	mesh;
	uint	material;
	uint	light;
	uint	fontColor;
	uint	texture;
} index;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureUV;
layout(location = 3) in uint textureIndex;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTextureUV;
layout(location = 3) flat out uint fragTextureIndex;


void main()
{
	vec4 worldPos = matrixUbo.view * meshData.modelMatrix[index.mesh] * vec4(position, 1.0f);
	fragNormal = normalize(mat3(meshData.normalMatrix[index.mesh]) * normal);
	fragPos = worldPos.xyz;

	fragTextureUV = textureUV;
	fragTextureIndex = textureIndex;

	gl_Position = matrixUbo.projection * worldPos;
}
