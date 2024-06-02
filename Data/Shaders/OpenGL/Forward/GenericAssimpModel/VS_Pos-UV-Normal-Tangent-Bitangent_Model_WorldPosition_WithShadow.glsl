#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

out VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 VertexNormal;
	mat3 TangentToWorld;
	vec3 VertexColor;
} vs_out;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

#define SH_NUM_BANDS 2
#define SH_COEFFICIENT_COUNT (SH_NUM_BANDS * SH_NUM_BANDS)

layout(std140, binding = 1) uniform GIBuffer
{
	vec4 LightCoeffs[SH_COEFFICIENT_COUNT];
}LightingBuffer;

layout(std140, binding = 2) uniform LightingUniforms
{
	int bHasNormalMap;
	int bOverrideColor;
	vec3 OverrideColor;
} LightingUniformsBuffer;

layout(binding = 0) uniform samplerBuffer tbo_texture;

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));
	vs_out.worldPos = fragPos;
	vs_out.TexCoords = inTexCoords;
	vs_out.clipToWorldMatrix = inverse(projection * view);
	vs_out.VertexNormal = mat3(transpose(inverse(model))) * inNormal;

	vec3 resColor = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < SH_COEFFICIENT_COUNT; ++i)
	{
		resColor += LightingBuffer.LightCoeffs[i].xyz * texelFetch(tbo_texture, gl_VertexID * SH_COEFFICIENT_COUNT + i).rgb;
	}
	vs_out.VertexColor = resColor;

	//vs_out.VertexColor = texelFetch(tbo_texture, gl_VertexID * SH_COEFFICIENT_COUNT).rgb;
	//vs_out.VertexColor = LightingUniformsBuffer.OverrideColor;


	//vs_out.VertexColor = LightingUniformsBuffer.OverrideColor;

	// Gram - Schmidt process
	// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
	vec3 T = normalize(vec3(model * vec4(inTangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(inNormal, 0.0)));
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);
	vs_out.TangentToWorld = mat3(T, B, N);

	gl_Position = projection * view * vec4(fragPos, 1.0);
}