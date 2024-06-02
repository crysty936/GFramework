#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

//layout(location = 0) out vec3 outNormal;
//layout(location = 0) out vec2 outTexCoords;

out VS_OUT
{
	vec2 TexCoords;
} vs_out;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 model;
};

void main()
{
	vs_out.TexCoords = inTexCoords;
	gl_Position = model * vec4(inPosition, 1.0);
}