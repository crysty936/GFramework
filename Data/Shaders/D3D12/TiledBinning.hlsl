#include "Utils.hlsl"

#define TILE_SIZE 16

// 16 byte aligned
struct ShaderDecal
{
	float4 Orientation;	// 16 bytes
	float3 Size;		// 28 bytes
	float3 Position;	// 40 bytes
	uint AlbedoMapIdx;	// 44 bytes
	uint NormalMapIdx;	// 48 bytes
};

StructuredBuffer<ShaderDecal> DecalBuffer : register(t0, space0);
Texture2D GBufferDepth : register(t1, space0);

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

RWByteAddressBuffer TilingBuffer : register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CSMain(in uint3 DispatchID : SV_DispatchThreadID, in uint GroupIndex : SV_GroupIndex,
            in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
	// DispatchID		- ThreadId relative to whole pass, equal to  GroupID * numthreads + GroupThreadID
	// GroupThreadID	- ThreadID relative to current group
	// GroupIndex		- Identical to GroupThreadID just that it is flattened
	// GroupID			- Index of the group. Depends on the dispatch numbers

	float2 pixelPos = DispatchID.xy;



		


}
