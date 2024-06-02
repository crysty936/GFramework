#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

out VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 Normal;
	vec3 DirectionalLightDirection;
	mat4 ShadowCameraViewMatrix;
	mat4 lsMatrices[3];
	flat int cascadesCount;
	flat int bVisualizeMode;
	flat float shadowCascadeFarPlanes[3];
} vs_out;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

layout(std140, binding = 1) uniform LightDataBuffer
{
	mat4 lsMatrices[3];
	mat4 ShadowCameraViewMatrix;
	vec4 DirectionalLightDirection;
	int cascadesCount;
	int bVisualizeMode;
	float shadowCascadeFarPlanes[3];
};

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));
	vs_out.lsMatrices = lsMatrices;
	vs_out.worldPos = fragPos;
	vs_out.TexCoords = inTexCoords;
	vs_out.clipToWorldMatrix = inverse(projection * view);
	vs_out.Normal = mat3(transpose(inverse(model))) * inNormal;
	vs_out.DirectionalLightDirection = DirectionalLightDirection.xyz;
	vs_out.ShadowCameraViewMatrix = ShadowCameraViewMatrix;
	vs_out.cascadesCount = cascadesCount;
	vs_out.bVisualizeMode = bVisualizeMode;
	vs_out.shadowCascadeFarPlanes = shadowCascadeFarPlanes;

	gl_Position = projection * view * vec4(fragPos, 1.0);
}