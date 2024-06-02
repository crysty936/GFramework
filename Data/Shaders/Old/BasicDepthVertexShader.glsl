#version 330 core 
layout(location = 0) in vec3 inPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 inNormal;

void main()
{
	gl_Position =  lightSpaceMatrix * model *  vec4(inPosition, 1.0);

	float constantBias = 0.005;
	gl_Position.z += constantBias;
}