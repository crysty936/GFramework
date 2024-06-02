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
	vec3 FragWorldPos;
	vec3 FragTangentPos;
	vec3 Normal;
	mat3 WorldToTangent;
	flat vec3 DirectionalLightDirectionTS;
	flat int bNormalVisualizeMode;
	flat int bUseNormalMapping;
	flat int bUseParallaxMapping;
	flat float ParallaxHeightScale;
	vec3 TSViewPos;
} vs_out;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

layout(std140, binding = 1) uniform ShadowDataBuffer
{
	vec4 DirectionalLightDirection;
	int bNormalVisualizeMode;
	int bUseNormalMapping;
	int bUseParallaxMapping;
	float ParallaxHeightScale;
	vec3 ViewPos;
};

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));
	vs_out.FragWorldPos = fragPos;
	vs_out.TexCoords = inTexCoords;
	vs_out.clipToWorldMatrix = inverse(projection * view);
	vs_out.Normal = mat3(transpose(inverse(model))) * inNormal;
	vs_out.bNormalVisualizeMode = bNormalVisualizeMode;
	vs_out.bUseNormalMapping = bUseNormalMapping;
	vs_out.bUseParallaxMapping = bUseParallaxMapping;
	vs_out.ParallaxHeightScale = ParallaxHeightScale;

	//// Gram - Schmidt process
	//// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
	vec3 T = normalize(vec3(model * vec4(inTangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(inNormal, 0.0)));
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);
	vs_out.WorldToTangent = transpose(mat3(T, B, N));

	vs_out.TSViewPos = vs_out.WorldToTangent * ViewPos;
	vs_out.FragTangentPos = vs_out.WorldToTangent * fragPos;
	vs_out.DirectionalLightDirectionTS = vs_out.WorldToTangent * DirectionalLightDirection.xyz;

	gl_Position = projection * view * vec4(fragPos, 1.0);
}