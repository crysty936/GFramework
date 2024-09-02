
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

// 256 byte aligned
cbuffer LightingConstantBuffer : register(b0)
{
	float4x4 ViewInv;
	float4x4 PerspInv;

    float4 ViewPos;
    float4 LightDir;
    float Padding[24];
};

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferRoughness : register(t2);
Texture2D GBufferDepth : register(t3);

SamplerState g_sampler : register(s0);


static const float PI = 3.14159265359;

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


// Trowbridge-Reitz
float DistributionGGX(float3 N, float3 H, float roughness)
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

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	// Shadowing from obstructions in the camera dir
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);

	// Shadowing from obstructions blocking light from exitting
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 CalcDirLight(float2 inTexCoords, float3 lightDir, float3 normal, float3 viewDir, float roughness, float metalness)
{
	float4 albedo = GBufferAlbedo.Sample(g_sampler, inTexCoords);
	float3 ambient = 0.2 * albedo.xyz;

	// diffuse shading
	float diff = clamp(dot(normal, -lightDir), 0.0, 1.0);
	float3 diffuse = diff * albedo.xyz;

	// specular shading
	// Blinn-Phong
	float3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
	float3 specular = spec * float3(albedo.a, albedo.a, albedo.a);
	return (ambient + diffuse + specular);
}

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;

    result.uv = uv;
    // Inverse y to simulate opengl like uv space
    result.uv.y = 1 - uv.y;

    result.position = position;    
    //result.position.x += offset;

    return result;
}

PSOutput PSMain(PSInput input)
{
    const float2 uv = input.uv;

    PSOutput output;
    float4 albedo = GBufferAlbedo.Sample(g_sampler, uv);

	float depth = GBufferDepth.Sample(g_sampler, uv).r;
	
	float2 clipCoord = uv * 2.f - 1.f;
	float4 clipLoc = float4(clipCoord, depth, 1.f);
	
	float4 viewSpacePos = mul(clipLoc, PerspInv);
	viewSpacePos /= viewSpacePos.w;
	
	const float4 worldSpacePos = mul(viewSpacePos, ViewInv);

	float3 wsNormal = GBufferNormal.Sample(g_sampler, uv).xyz;
	wsNormal = wsNormal * 2.f - 1.f;

	const float3 viewToFrag = normalize(worldSpacePos.xyz - ViewPos.xyz);
	const float3 fragToViewW = -viewToFrag;

	const float metalness = GBufferRoughness.Sample(g_sampler, uv).r;
	const float roughness = GBufferRoughness.Sample(g_sampler, uv).r;
	
	float3 DirLightRadiance = 0;


	{
		float3 albedo = GBufferAlbedo.Sample(g_sampler, uv).xyz;
		float3 N = wsNormal;
		float3 V = fragToViewW;

		float3 L = -LightDir.xyz;

		// Halfway vector
		float3 H = normalize(V + L);

		// Cook-Torrance BRDF

		// For dielectric, use general 0.04 for Surface Reflection F0(How much the surface reflects if looking at it perpendicularly
		// while for metallics, albedo will give the value
		float3 F0 = 0.04;
		F0 = lerp(F0, albedo, metalness);

		// Specular

		// Fresnel
		float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		// Normal Distribution Function( Approcimates the amount of microfacets aligned to the halway vector)
		float NDF = DistributionGGX(N, H, roughness);

		// Approximates self-shadowing by the microfacets(both by view occlusion and light capture)
		float G = GeometrySmith(N, V, L, roughness);

		//(D * F * G)
		float3 numerator = NDF * F * G;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		float3 specular = numerator / denominator;

		// Rest of radiance is refracted light(diffuse)
		float3 kS = F;
		float3 Kd = float3(1.f, 1.f, 1.f) - kS;
		Kd *= 1.0 - metalness;

		float3 lightIntenstiy = float3(2.0, 2.0, 2.0);

		float NdotL = max(dot(N, L), 0.0);
		DirLightRadiance = (Kd * albedo / PI + specular) * lightIntenstiy * NdotL;

		float3 ambient = 0.03 * albedo;
		DirLightRadiance += ambient;

	}


	float3 color = DirLightRadiance;

	// reinhard tone mapping
	color = color / (color + float3(1.f, 1.f, 1.f));

	const float gamma = 2.2;
	const float inverseGamma = 1.0 / gamma;
	// gamma correction 
	color = pow(color, float3(inverseGamma, inverseGamma, inverseGamma));

	float3 finalColor = color;	

	//const float cameraNear = 0.1f;
	//const float cameraFar = 10000.f;
	// Transform depth into view space
	// Basically inverse of projection, only applied to z
	//float3 linearizedDepth = (cameraNear * cameraFar) / (cameraFar +  depth * (cameraNear - cameraFar));
	//linearizedDepth = linearizedDepth / 50;
	//output.Color = float4(linearizedDepth, 1.0);

	output.Color = float4(finalColor * 1.5, 1);
	//output.Color = albedo;

    return output;
}
