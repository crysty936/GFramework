#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

#define TILE_SIZE 32

// 16 byte aligned
struct ShaderDecal
{
	float4 Orientation;	// 16 bytes
	float3 Size;		// 28 bytes
	float3 Position;	// 40 bytes
	uint AlbedoMapIdx;	// 44 bytes
	uint NormalMapIdx;	// 48 bytes
};

StructuredBuffer<ShaderDecal> DecalBuffer : register(t0, space100);
Texture2D GBufferDepth : register(t1, space100);

// 256 byte aligned
struct DecalConstantBuffer
{
    float4x4 Projection;
    float4x4 View;
	float4x4 InvViewProj;

	float Padding[16];
};

// 256 byte aligned
ConstantBuffer<DecalConstantBuffer> ConstBuffer : register(b0);

SamplerState g_sampler : register(s0);

RWTexture2D<float4> OutputAlbedo : register(u0);
RWTexture2D<float4> OutputNormal : register(u1);
RWTexture2D<float4> OutputMetallicRoughness : register(u2);


[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CSMain(in uint3 DispatchID : SV_DispatchThreadID, in uint GroupIndex : SV_GroupIndex,
            in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
	float2 pixelPos = DispatchID.xy;

// 	float3 groupColor = GroupThreadID * float3(0.01f, 0.01f, 0.01f);
// 	float3 groupColor = GroupID * float3(0.01f, 0.01f, 0.01f);
// 
// 	OutputAlbedo[pixelPos] = float4(groupColor, 1.f);
// 	return;	

	ShaderDecal usedDecal = DecalBuffer[0];


	int2 TextureSize;
	GBufferDepth.GetDimensions(TextureSize.x, TextureSize.y);
	const float2 Resolution = float2(TextureSize.x, TextureSize.y);
	const float2 TexelSize = 1 / Resolution;
	const float2 UV = TexelSize * (pixelPos + 0.5f);

	const float clipDepth = GBufferDepth[pixelPos].r;

	//float linearizedDepth = SceneBuffer.Projection._43 / (clipDepth - SceneBuffer.Projection._33); 

	float2 clipCoord = UV * 2.f - 1.f;
	float4 clipPos = float4(clipCoord, clipDepth, 1.f);
	clipPos.y *= -1.f; // NDC coordinates for D3D start from upper left corner, so X stays the same but Y is flipped

	const float4 worldPosHom = mul(clipPos, ConstBuffer.InvViewProj);
	const float4 worldPos = worldPosHom / worldPosHom.w;	
	
	//OutputAlbedo[pixelPos] = worldPos;
	//return;

	float3x3 decalRot = QuatTo3x3(usedDecal.Orientation);

	float3 localPos = worldPos.xyz - usedDecal.Position;
	localPos = mul(localPos, transpose(decalRot));
	float3 decalUVW = localPos / usedDecal.Size;
	decalUVW.y *= -1;
	
	[branch]
	if (
		decalUVW.x >= -1.f && decalUVW.x <= 1.f &&
		decalUVW.y >= -1.f && decalUVW.y <= 1.f &&
		decalUVW.z >= -1.f && decalUVW.z <= 1.f)
	{
	    Texture2D AlbedoMap = Tex2DTable[usedDecal.AlbedoMapIdx];
		//OutputTexture[pixelPos] = float4(1.f, 0.f, 0.f, 1.f);
		OutputAlbedo[pixelPos] = AlbedoMap.SampleLevel(g_sampler, decalUVW.xy, 0);
		
	    Texture2D NormalMap = Tex2DTable[usedDecal.NormalMapIdx];

		float3 decalNormalTS = NormalMap.SampleLevel(g_sampler, decalUVW.xy, 0).xyz;
		//float3 decalNormalTS = float3(0.f, 0.f, 1.f);
		decalNormalTS *= 0.5f + 0.5f;
		
		decalNormalTS.z *= -1.f;
		
		float3 decalNormalWS = mul(decalNormalTS, decalRot);
		decalNormalWS *= 2.f - 1.f;
		
		OutputNormal[pixelPos] = float4(decalNormalWS, 1.f);
	}


}
