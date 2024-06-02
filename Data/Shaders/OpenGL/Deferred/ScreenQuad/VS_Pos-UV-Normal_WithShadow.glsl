#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));

	gl_Position = projection * view * vec4(fragPos, 1.0);
}