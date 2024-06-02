#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in GS_OUT{
	vec3 color;
}  ps_in;


void main()
{
	FragColor = vec4(ps_in.color, 1.0);
}