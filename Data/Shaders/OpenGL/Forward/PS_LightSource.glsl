#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
	FragColor = color;
}