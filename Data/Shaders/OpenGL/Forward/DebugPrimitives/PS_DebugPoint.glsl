#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec4 Color;
} vs_out;

void main()
{
	vec4 color = vs_out.Color;;
	FragColor = color;
}