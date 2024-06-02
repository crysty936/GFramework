#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

out VS_OUT
{
	vec3 Normal;
} vs_out;

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));
	vs_out.Normal = mat3(transpose(inverse(model))) * inNormal;
	gl_Position = projection * view * vec4(fragPos, 1.0);
}