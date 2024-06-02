#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedo;
layout(location = 2) out vec4 gMetallicRoughness;

in VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 VertexNormal;
	mat3 TangentToWorld;
	vec3 modelPos;
} ps_in;

layout(std140, binding = 1) uniform CascadedShadowDataBuffer
{
	float Metallic;
	float Roughness;
} MatProperties;


const float PI = 3.14159265359f;

void main()
{
	gNormal = vec4(ps_in.VertexNormal / 2.0 + 0.5, 1.0);
	//gAlbedo = vec4(1.0, 0.0, 0.0, 1.0);

	const vec3 relPos = ps_in.modelPos;

	const float ro = length(relPos);
	const float theta = acos(relPos.z / ro);
	const float phi = atan(relPos.y / relPos.x);

	const float normalizedTheta = theta / PI;
	const float normalizedPhi = phi / PI;

	//gAlbedo = vec4(normalizedTheta, normalizedTheta, normalizedTheta, 1.0);

	if (theta < (PI / 6.f))
	{
		gAlbedo = vec4(1, 1, 1, 1.0);
	}
	else
	{
		gAlbedo = vec4(0, 0, 0, 1.0);
	}

	gMetallicRoughness = vec4(MatProperties.Metallic, MatProperties.Roughness, 0.0, 0.0);
}