#version 450

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
	mat4	orthographic;
}	matrixUbo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureUV;

layout(location = 0) out vec2 fragTextureUV;


void main()
{
	fragTextureUV = textureUV;
	gl_Position = matrixUbo.orthographic * vec4(position, 1.0f);
}
