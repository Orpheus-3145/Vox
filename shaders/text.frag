#version 450

// default value if not overridden
layout(constant_id = 0) const uint MAX_FONT_COLORS = 2;

layout(push_constant) uniform DescriptorIndexes {
	uint	mesh;
	uint	material;
	uint	light;
	uint	fontColor;
	uint	texture;
} index;

layout(set = 1, binding = 0) uniform FontUBO {
	vec4	color;
} fontColors[MAX_FONT_COLORS];

layout(set = 1, binding = 1) uniform sampler2D fontSampler;

layout(location = 0) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	float alpha = texture(fontSampler, fragTextureUV).r;
	outColor = vec4(fontColors[index.fontColor].color.rgb, fontColors[index.fontColor].color.a * alpha);
}
