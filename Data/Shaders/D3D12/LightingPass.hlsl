
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
    float4 ViewPos;
    float4 LightDir;
    float Padding[56];
};

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferRoughness : register(t2);

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
    float2 uv = input.uv;

    PSOutput output;
    //float4 albedo = GBufferAlbedo.Sample(g_sampler, uv);
    //output.Color = albedo + (0.1f * LightDir);



	

    return output;
}
