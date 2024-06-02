#version 330 core 
layout(location = 0) out vec4 FragColor;

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
	float fragmentDepth = LinearizeDepth(gl_FragCoord.z) / far;

	FragColor= vec4(vec3(fragmentDepth), 1.0);
}