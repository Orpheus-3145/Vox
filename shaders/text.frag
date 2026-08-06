#version 450

// default value if not overridden
layout(constant_id = 0) const uint MAX_OBJS = 8;
layout(constant_id = 1) const uint MAX_MATERIALS = 8;
layout(constant_id = 2) const uint MAX_LIGHTS = 8;
layout(constant_id = 3) const uint MAX_TEXT_COLORS = 8;
layout(constant_id = 4) const uint MAX_TEXTURES = 2;

layout(set = 1, binding = 0) uniform TextDataUBO
{
	vec4	color[MAX_TEXT_COLORS];
}	textUbo;

layout(set = 1, binding = 1) uniform sampler2D fontSampler;

layout(push_constant) uniform DescriptorIndexes {
	uint	mesh;
	uint	material;
	uint	light;
	uint	fontColor;
	uint	texture;
} index;

layout(location = 0) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	float alpha = texture(fontSampler, fragTextureUV).r;
	outColor = vec4(textUbo.color[index.fontColor].rgb, textUbo.color[index.fontColor].a * alpha);
}
