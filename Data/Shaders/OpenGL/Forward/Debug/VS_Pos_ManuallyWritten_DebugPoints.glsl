#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 projection;
	mat4 view;
};


void main()
{
	gl_Position = projection * view * vec4(inPosition, 1.0);
	
	// Shader mostly used for debug points, make sure they're always in front
	gl_Position.z = 0.f;
}