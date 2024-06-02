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
} vs_out;

layout(std140, binding = 0) uniform GeometryDataBuffer
{
	mat4 projection;
	mat4 view;
	mat4 model;
};

void main()
{
	vec3 fragPos = vec3(model * vec4(inPosition, 1.0));
	vs_out.worldPos = fragPos;
	vs_out.TexCoords = inTexCoords;
	vs_out.clipToWorldMatrix = inverse(projection * view);
	vs_out.VertexNormal = mat3(transpose(inverse(model))) * inNormal;

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