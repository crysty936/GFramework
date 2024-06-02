#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in mat4 instanceMatrix;

out VS_OUT
{
	vec2 TexCoords;
} vs_out;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

void main()
{
	vs_out.TexCoords = inTexCoords;
	gl_Position = projection * view * instanceMatrix * model * vec4(inPosition, 1.0);
}