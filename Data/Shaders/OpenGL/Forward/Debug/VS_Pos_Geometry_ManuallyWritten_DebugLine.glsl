#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inStartPosition;
layout(location = 1) in vec3 inEndPosition;
layout(location = 2) in vec3 inColor;

out VS_OUT{
	vec4 endPosition;
	vec3 color;
} vs_out;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 projection;
	mat4 view;
};

void main()
{
	vs_out.color = inColor;
	vs_out.endPosition = projection * view * vec4(inEndPosition, 1.0);

	gl_Position = projection * view * vec4(inStartPosition, 1.0);
}