#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

layout(std140, binding = 1) uniform ViewConstantsBuffer
{
	mat4 perspInv;
	mat4 viewInv;
	vec4 CameraNearFar;
	vec4 ViewportSize;

} ViewConstants;

layout(std140, binding = 2) uniform LightingDataBuffer
{
	int bUseDirLight;
	vec3 DirectionalLightDir;
	vec3 ViewPos;
} LightingConstants;

layout(binding = 0) uniform sampler2D GBufferAlbedo;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferMetallicRoughness;
layout(binding = 3) uniform sampler2D GBufferDepth;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Trowbridge-Reitz
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);

	denom = PI * denom * denom;

	return num / denom;
}

// Smith's Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	
	return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	// Shadowing from obstructions in the camera dir
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);

	// Shadowing from obstructions blocking light from exitting
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

void main()
{
	const float depthValue = texture(GBufferDepth, ps_in.TexCoords).r;
	FragColor = vec4(vec3(depthValue), 1.0);

	vec3 wsNormal = texture(GBufferNormal, ps_in.TexCoords).rgb;
	wsNormal = wsNormal * 2.0 - 1.0;
	wsNormal = normalize(wsNormal);
	//FragColor = vec4(wsNormal, 1.0);

	vec2 clipCoord = ps_in.TexCoords.xy * 2.0 - 1.0;
	vec4 clipsSpaceLoc = vec4(clipCoord.x, clipCoord.y, depthValue, 1.0);

	vec4 viewSpacePosition = ViewConstants.perspInv * clipsSpaceLoc;
	viewSpacePosition /= viewSpacePosition.w;
	vec4 worldSpacePos = ViewConstants.viewInv * viewSpacePosition;
	//vec3 albedo = texture(GBufferAlbedoSpec, ps_in.TexCoords).rgb;
	//FragColor = vec4(albedo, 1.0);

	vec3 viewToFragW = normalize(worldSpacePos.xyz - LightingConstants.ViewPos);
	vec3 fragToViewW = -viewToFragW;

	float metalness = texture(GBufferMetallicRoughness, ps_in.TexCoords).r;
	float roughness = texture(GBufferMetallicRoughness, ps_in.TexCoords).g;

	vec3 DirLightRadiance = vec3(0.0);

	{

		vec3 albedo = texture(GBufferAlbedo, ps_in.TexCoords).rgb;
		vec3 N = wsNormal;
		vec3 V = fragToViewW;

		vec3 L = -LightingConstants.DirectionalLightDir;

		// Halfway vector
		vec3 H = normalize(V + L);

		// Cook-Torrance BRDF

		// For dielectric, use general 0.04 for Surface Reflection F0(How much the surface reflects if looking at it perpendicularly
		// while for metallics, albedo will give the value
		vec3 F0 = vec3(0.04);
		F0 = mix(F0, albedo, metalness);

		// Specular

		// Fresnel
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		// Normal Distribution Function( Approcimates the amount of microfacets aligned to the halway vector)
		float NDF = DistributionGGX(N, H, roughness);

		// Approximates self-shadowing by the microfacets(both by view occlusion and light capture)
		float G = GeometrySmith(N, V, L, roughness);

		//(D * F * G)
		vec3 numerator = NDF * F * G;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// Rest of radiance is refracted light(diffuse)
		vec3 kS = F;
		vec3 Kd = vec3(1.0) - kS;
		Kd *= 1.0 - metalness;

		vec3 lightIntenstiy = vec3(2.0, 2.0, 2.0);

		float NdotL = max(dot(N, L), 0.0);
		DirLightRadiance = (Kd * albedo / PI + specular) * lightIntenstiy * NdotL;

		vec3 ambient = vec3(0.03) * albedo;
		DirLightRadiance += ambient;
	}


	vec3 colorHDR = DirLightRadiance;

	// reinhard tone mapping
	vec3 mapped = colorHDR / (colorHDR + vec3(1.0));

	const float gamma = 2.2;
	// gamma correction 
	mapped = pow(mapped, vec3(1.0 / gamma));

	vec3 finalColor = mapped;

	FragColor = vec4(finalColor, 1.0);
}