#version 330 core 
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPosition;
layout(location = 0) out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	//aPosition = vec3(aPosition.x, APosition.)

	TexCoords = aPosition;

	vec4 pos= projection * view * vec4(aPosition, 1.0);

	// Set the skybox at the farthest position in the frustum so that it's rendered behind everything else
	gl_Position = pos.xyww;
}