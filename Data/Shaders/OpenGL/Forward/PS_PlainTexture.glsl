#version 330 core
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec2 inTexCoords;

uniform sampler2D quadTexture;

void main()
{
	// For drawing from a texture that contains depth only
	//float depthValue = texture(quadTexture, inTexCoords).r;
	//FragColor = vec4(vec3(depthValue), 1.0);

	vec4 color = texture(quadTexture, inTexCoords);
	FragColor = color;
}