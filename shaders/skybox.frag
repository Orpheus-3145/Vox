#version 450

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

layout(set = 1, binding = 0) uniform sampler2D samplers[MAX_TEXTURES];
layout(set = 1, binding = 1) uniform samplerCube skySampler;

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;


void main()
{
	vec3 dir = fragDir;
	dir.x = -dir.x;
	outColor = texture(skySampler, dir);
}
