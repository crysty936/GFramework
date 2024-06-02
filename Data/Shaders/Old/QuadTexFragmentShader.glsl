#version 330 core 
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec2 inTexCoords;

uniform sampler2D QuadTexture;

float near = 0.1;
float far = 1000.0;

float LinearizeDepth(float inDepth)
{
	float z = inDepth * 2.0 - 1.0;
	float farExtracted = (2.0 * near * far) / (far + near - z * (far - near));

	return farExtracted;
}

void main()
{
	// For drawing from a texture that contains depth only
	float depthValue = texture(QuadTexture, inTexCoords).r;
	FragColor = vec4(vec3(depthValue), 1.0);

	//float depthValue = texture(QuadTexture, inTexCoords).r;
	//float fragmentDepth = LinearizeDepth(depthValue) / far;

	//FragColor = vec4(vec3(fragmentDepth), 1.0);


	//vec4 color = texture(QuadTexture, inTexCoords);
	//FragColor = color;
}