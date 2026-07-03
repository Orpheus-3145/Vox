#version 450

layout(set = 1, binding = 0) uniform sampler2D fontSampler;

layout(location = 0) in vec2 fragTextureUV;

layout(location = 0) out vec4 outColor;

void main()
{
	// NB tmp
	vec4 textColor = {0, 1, 0, 1};
	float alpha = texture(fontSampler, fragTextureUV).r;
	outColor = vec4(textColor.rgb, textColor.a * alpha);
}
