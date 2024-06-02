#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

out VS_OUT
{
	vec2 TexCoords;
} vs_out;

void main()
{
	vs_out.TexCoords = inTexCoords;
	gl_Position = vec4(inPosition, 1.0);
}