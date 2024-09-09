#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

// 16 byte aligned
struct ShaderDecal
{
	float4 Orientation;	// 16 bytes
	float3 Size;		// 28 bytes
	float3 Position;	// 40 bytes
	uint AlbedoTexIdx;	// 44 bytes
	uint NormalTexIdx;	// 48 bytes
};

StructuredBuffer<ShaderDecal> DecalBuffer : register(t0, space100);
Texture2D GBufferDepth : register(t1, space100);

// 256 byte aligned
struct DecalConstantBuffer
{
    float4x4 Projection;
    float4x4 View;

	float Padding[32];
};

struct DebugIndexBuffer
{
    uint DebugMaterialIdx;
};

// 256 byte aligned
ConstantBuffer<DecalConstantBuffer> SceneBuffer : register(b0);
ConstantBuffer<DebugIndexBuffer> MatIndex : register(b1);

SamplerState g_sampler : register(s0);

RWTexture2D<float4> OutputTexture : register(u0);

[numthreads(NUM_THREADS_PER_GROUP_DIMENSION, NUM_THREADS_PER_GROUP_DIMENSION, 1)]
void CSMain(in uint3 DispatchID : SV_DispatchThreadID, in uint GroupIndex : SV_GroupIndex,
            in uint GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
	int2 TextureSize;
	OutputTexture.GetDimensions(TextureSize.x, TextureSize.y);
	const float2 Resolution = float2(TextureSize.x, TextureSize.y);
	const float2 TexelSize = 1 / Resolution;
	const float2 UV = TexelSize * (DispatchID.xy + 0.5f);


	OutputTexture[DispatchID.xy] = float4(1.f, 0.f, 0.f, 1.f);

}
