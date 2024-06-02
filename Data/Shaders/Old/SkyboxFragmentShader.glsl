#version 330 core 
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 inTexCoords;
layout(location = 0) out vec4 FragColor;

uniform samplerCube CubeMap;

void main()
{
	vec4 color = texture(CubeMap, inTexCoords);

	FragColor = color;
}