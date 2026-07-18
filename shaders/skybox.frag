#version 450

// default values if not overridden
layout(constant_id = 0) const uint MAX_OBJS = 8;
layout(constant_id = 1) const uint MAX_MATERIALS = 8;
layout(constant_id = 2) const uint MAX_LIGHTS = 8;
layout(constant_id = 3) const uint MAX_FONTS = 8;
layout(constant_id = 4) const uint MAX_TEXTURES = 8;

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
