#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec4 instanceColor;
layout(location = 3) in mat4 instanceMat;

layout(std140, binding = 0) uniform ConstantBuffer
{
	mat4 projection;
	mat4 view;
};

layout(std140, binding = 1) uniform BillboardBuffer
{
	mat4 cameraLookAt;
};

out VS_OUT
{
	vec4 Color;
} vs_out;

void main()
{
	//gl_Position = projection * view * cameraLookAt * vec4(instancePos + inPosition, 1.0);
	gl_Position = projection * view /** cameraLookAt */* instanceMat * vec4(inPosition, 1.0);

	vs_out.Color = instanceColor;
	
	// Shader used for debug points, make sure they're always in front
	//gl_Position.z = 0.f;
}