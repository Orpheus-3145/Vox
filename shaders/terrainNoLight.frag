#version 450

layout(set = 1, binding = 0) uniform sampler2D textSampler;

layout(location = 2) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(textSampler, fragTextureUV);
}
