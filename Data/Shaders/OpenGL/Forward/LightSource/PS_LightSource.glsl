#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

layout(std140, binding = 1) uniform LightBuffer
{
	vec3 Color;
} lBuffer;


void main()
{
	vec4 color = vec4(lBuffer.Color, 1.0);
	FragColor = color;
}