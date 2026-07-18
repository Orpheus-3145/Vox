#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(constant_id = 0) const uint MAX_OBJS = 8;
layout(constant_id = 1) const uint MAX_MATERIALS = 8;
layout(constant_id = 2) const uint MAX_LIGHTS = 8;
layout(constant_id = 3) const uint MAX_FONTS = 8;
layout(constant_id = 4) const uint MAX_TEXTURES = 8;

layout(push_constant) uniform DescriptorIndexes {
	uint	mesh;
	uint	material;
	uint	light;
	uint	fontColor;
	uint	texture;
} index;

layout(set = 1, binding = 0) uniform sampler2D samplers[MAX_TEXTURES];

layout(location = 0) in vec2 fragTextureUV;
layout(location = 1) flat in uint fragTextureIndex;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(samplers[nonuniformEXT(index.texture + fragTextureIndex)], fragTextureUV);
}
