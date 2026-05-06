#version 450

layout(set = 0, binding = 0) uniform MatrixUBO
{
	mat4	view;
	mat4	projection;
}	ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureUV;

layout(location = 0) out vec3 fragDir;


void main()
{
	fragDir = inPosition;
	mat4 viewWithoutTranslation = mat4(mat3(ubo.view));
	vec4 pos = ubo.projection * viewWithoutTranslation * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;		// forcing fragment of far plane (depth = 1.0)
}
