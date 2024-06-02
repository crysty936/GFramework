#version 330 core 

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 inTexCoords;

//out vec3 INormal;
//out vec3 IFragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	TexCoords = inTexCoords;

	gl_Position = vec4(aPosition.x, aPosition.y, 0.0 , 1.0);
}