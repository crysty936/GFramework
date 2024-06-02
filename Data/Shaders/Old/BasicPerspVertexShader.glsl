#version 330 core 
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	outTexCoords = inTexCoords;
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));

	gl_Position= projection * view * vec4(fragPos, 1.0);
}