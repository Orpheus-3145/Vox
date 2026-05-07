#version 450

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

layout(constant_id = 0) const uint nMaterials = 8;  // default if not overridden
layout(constant_id = 1) const uint nLights = 8; 	// default if not overridden

layout(set = 0, binding = 1) uniform MaterialUBO {
	MaterialData	materials[nMaterials];
	LightData		lights[nLights];
} colorData;

layout(set = 1, binding = 0) uniform sampler2D textSampler;

layout(push_constant) uniform MeshData {
	mat4	modelMatrix;
	mat4	normalMatrix;
	int		materialIndex;
	int		lightIndex;
} meshData;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 diffuseColor = texture(textSampler, fragTextureUV);

	// Ambient
	vec3 ambient = colorData.lights[meshData.lightIndex].lightAmbientColor.xyz * diffuseColor.xyz;

	// Diffuse
	float diff = max(dot(fragNormal, colorData.lights[meshData.lightIndex].lightDir.xyz), 0.0);
	vec3 diffuse = colorData.lights[meshData.lightIndex].lightColor.xyz * diff * diffuseColor.xyz;

	// Specular (Blinn-Phong)
	vec3 viewDir = normalize(-fragPos);
	vec3 halfwayDir = normalize(colorData.lights[meshData.lightIndex].lightDir.xyz + viewDir);
	float spec = pow(max(dot(fragNormal, halfwayDir), 0.0), colorData.materials[meshData.materialIndex].shininess);
	vec3 specular = colorData.lights[meshData.lightIndex].lightSpecularColor.xyz * spec * colorData.materials[meshData.materialIndex].specularColor.xyz;

	vec3 result = ambient + diffuse + specular;
	outColor = vec4(result, colorData.materials[meshData.materialIndex].opacity);
}
