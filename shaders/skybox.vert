#version 450

layout(set = 0, binding = 0) uniform ViewProjectUBO
{
	mat4	view;
	mat4	projection;
	mat4	orthographic;
}	matrixUbo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragDir;


void main()
{
	fragDir = inPosition;
	mat4 viewWithoutTranslation = mat4(mat3(matrixUbo.view));
	vec4 pos = matrixUbo.projection * viewWithoutTranslation * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;		// forcing fragment of far plane (depth = 1.0)
}
