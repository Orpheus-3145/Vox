#version 450

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
}	matrixUbo;

// default values if not overridden
layout(constant_id = 0) const uint MAX_OBJS = 8;
layout(constant_id = 1) const uint MAX_MATERIALS = 8;
layout(constant_id = 2) const uint MAX_LIGHTS = 8;
layout(constant_id = 3) const uint MAX_TEXTURES = 8;

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
	uint	texture;
} index;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureUV;
layout(location = 3) in uint textureIndex;

layout(location = 0) out vec3 fragDir;


void main()
{
	fragDir = inPosition;
	mat4 viewWithoutTranslation = mat4(mat3(matrixUbo.view));
	vec4 pos = matrixUbo.projection * viewWithoutTranslation * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;		// forcing fragment of far plane (depth = 1.0)
}
