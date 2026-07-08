#version 460
#extension GL_EXT_nonuniform_qualifier : require

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
	MaterialData	material[MAX_MATERIALS];
	LightData		light[MAX_LIGHTS];
} meshData;

layout(push_constant) uniform DescriptorIndexes {
	uint	mesh;
	uint	material;
	uint	light;
	uint	fontColor;
	uint	texture;
} index;

layout(set = 1, binding = 0) uniform sampler2D samplers[MAX_TEXTURES];
layout(set = 1, binding = 1) uniform samplerCube skySampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTextureUV;
layout(location = 3) flat in uint fragTextureIndex;

layout(location = 0) out vec4 outColor;

void main()
{
	// only index thay changes between primitives, use nonuniformEXT to avoid
	// the compiler setting statically the index value for the whole subgroup
	vec4 diffuseColor = texture(samplers[nonuniformEXT(index.texture + fragTextureIndex)], fragTextureUV);

	MaterialData material = meshData.material[index.material];
	LightData light = meshData.light[index.light];

	// Ambient
	vec3 ambient = light.lightAmbientColor.xyz * diffuseColor.xyz;

	// Diffuse
	float diff = max(dot(fragNormal, light.lightDir.xyz), 0.0);
	vec3 diffuse = light.lightColor.xyz * diff * diffuseColor.xyz;

	// Specular (Blinn-Phong)
	vec3 viewDir = normalize(-fragPos);
	vec3 halfwayDir = normalize(light.lightDir.xyz + viewDir);
	float spec = pow(max(dot(fragNormal, halfwayDir), 0.0), material.shininess);
	vec3 specular = light.lightSpecularColor.xyz * spec * material.specularColor.xyz;

	vec3 result = ambient + diffuse + specular;
	outColor = vec4(result, material.opacity);
}
