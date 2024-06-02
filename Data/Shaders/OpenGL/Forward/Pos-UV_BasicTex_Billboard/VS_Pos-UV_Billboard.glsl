#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

out VS_OUT
{
	vec2 TexCoords;
} vs_out;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

layout(std140, binding = 1) uniform BillboardBuffer
{
	mat4 cameraLookAt;
};


void main()
{
	vs_out.TexCoords = inTexCoords;
	const vec4 worldPos = cameraLookAt * model * vec4(inPosition, 1.0);
	gl_Position = projection * view *  worldPos;
}