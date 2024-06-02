#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in mat4 instanceMatrix;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 model;
};

void main()
{
	gl_Position = instanceMatrix * model * vec4(inPosition, 1.0);
	// Stick objects behind the near plane on it so that they are drawing in the depth map
// 	if (gl_Position.z / gl_Position.w < 0.f)
// 	{
// 		gl_Position.z = 0.f;
// 	}

	//gl_Position.z = 1 - gl_Position.z;

}