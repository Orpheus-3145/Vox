#version 450

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec3 fragDir;
layout(location = 0) out vec4 outColor;


void main()
{
	vec3 dir = fragDir;
	dir.x = -dir.x;
	vec3 absDir = abs(dir);
	if (absDir.y > absDir.x && absDir.y > absDir.z)
	{
		dir = vec3(-dir.x, dir.y, -dir.z); // turn by 180° the top cube face
	}
	outColor = texture(skybox, dir);
}
