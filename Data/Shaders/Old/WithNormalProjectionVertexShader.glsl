#version 330 core 
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out vec4 outFragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
	outTexCoords = inTexCoords;
	outFragPos = vec3(model * vec4(inPosition, 1.0));
	outNormal = transpose(inverse(mat3(model))) * inNormal;
	outFragPosLightSpace = lightSpaceMatrix * vec4(outFragPos, 1.0);

	gl_Position= projection * view * vec4(outFragPos, 1.0);
}