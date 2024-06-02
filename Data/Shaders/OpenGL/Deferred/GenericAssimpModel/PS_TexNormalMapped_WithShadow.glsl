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
} ps_in;


layout(std140, binding = 1) uniform LightingUniforms
{
	int bHasNormalMap;
	int bOverrideColor;
	vec3 OverrideColor;
} LightingUniformsBuffer;


layout(binding = 0) uniform sampler2D Albedo;
layout(binding = 1) uniform sampler2D NormalMap;
layout(binding = 2) uniform sampler2D MetallicRoughness;

void main()
{
	vec3 wsNormal;

	if (bool(LightingUniformsBuffer.bHasNormalMap))
	{
		vec3 mapNormal = texture(NormalMap, ps_in.TexCoords).xyz;
		mapNormal = mapNormal * 2.0 - 1.0;
		wsNormal = normalize(ps_in.TangentToWorld * mapNormal);
	}
	else
	{
		wsNormal = normalize(ps_in.VertexNormal);
	}

	gNormal = vec4(wsNormal / 2.0 + 0.5, 1.0);
	
	
	if (bool(LightingUniformsBuffer.bOverrideColor))
	{
		vec4 albedo = vec4(LightingUniformsBuffer.OverrideColor.rgb, 1.0);
		vec3 albedoLinearized = pow(albedo.rgb, vec3(2.2));
		//vec3 albedoLinearized = albedo.xyz;
		gAlbedo = vec4(albedoLinearized, albedo.a);
	}
	else
	{
		vec4 albedo = texture(Albedo, ps_in.TexCoords);
		// Linearize
		vec3 albedoLinearized = pow(albedo.rgb, vec3(2.2));
		gAlbedo = vec4(albedoLinearized, albedo.a);
	}

	if (bool(LightingUniformsBuffer.bOverrideColor))
	{
		gMetallicRoughness = vec4(0, 1, 0, 0);
	}
	else
	{
		gMetallicRoughness = texture(MetallicRoughness, ps_in.TexCoords);
	}
}