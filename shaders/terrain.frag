#version 450

layout(set = 0, binding = 1) uniform LightUBO {
	vec4 lightDir;			// has to got FROM fragment TO to the light source, so has to negated on CPU side
	vec4 lightAmbientColor;
	vec4 lightColor;
	vec4 lightSpecularColor;
} lightUBO;

layout(set = 1, binding = 0) uniform sampler2D textSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform MeshData {
	mat4 modelMatrix;
	mat4 normalMatrix;

	vec4 ambientClr;
	vec4 diffuseClr;
	vec4 specularClr;
	float shininess;
	float opacity;
	int refractionIndex;
	int illuminationModel;
} meshData;

void main()
{
	vec4 diffuseColor = texture(textSampler, fragTextureUV);

	// Ambient
	vec3 ambient = lightUBO.lightAmbientColor.xyz * diffuseColor.xyz;

	// Diffuse
	float diff = max(dot(fragNormal, lightUBO.lightDir.xyz), 0.0);
	vec3 diffuse = lightUBO.lightColor.xyz * diff * diffuseColor.xyz;

	// Specular (Blinn-Phong)
	vec3 viewDir = normalize(-fragPos);
	vec3 halfwayDir = normalize(lightUBO.lightDir.xyz + viewDir);
	float spec = pow(max(dot(fragNormal, halfwayDir), 0.0), meshData.shininess);
	vec3 specular = lightUBO.lightSpecularColor.xyz * spec * meshData.specularClr.xyz;

	vec3 result = ambient + diffuse + specular;
	outColor = vec4(result, meshData.opacity);
}
