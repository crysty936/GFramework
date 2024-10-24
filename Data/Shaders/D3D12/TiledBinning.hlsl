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

StructuredBuffer<ShaderDecal> DecalBuffer : register(t0, space0);
Texture2D GBufferDepth : register(t1, space0);

// 256 byte aligned
struct DecalTilingConstantBuffer
{
    float4x4 Projection;
    float4x4 View;
	float4x4 InvViewProj;
	uint NumDecals;
	uint2 NumWorkGroups;
	uint DebugFlag;
	float DebugValue;

	float Padding[11];
};

// 256 byte aligned
ConstantBuffer<DecalTilingConstantBuffer> ConstBuffer : register(b0);

SamplerState g_sampler : register(s0);

RWByteAddressBuffer TilingBuffer : register(u0);


groupshared uint minDepthUint;
groupshared uint maxDepthUint;
groupshared uint visibleDecalsCount;
groupshared float4 frustumPlanes[6];
groupshared uint visibleDecalIndices[1024];

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CSMain(in uint3 DispatchID : SV_DispatchThreadID, in uint GroupIndex : SV_GroupIndex,
            in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
	// DispatchID		- ThreadId relative to whole pass, equal to  GroupID * numthreads + GroupThreadID
	// GroupThreadID	- ThreadID relative to current group
	// GroupIndex		- Identical to GroupThreadID just that it is flattened
	// GroupID			- Index of the group. Depends on the dispatch numbers

	float2 pixelPos = DispatchID.xy;

	// Set default values for whole group
	if (GroupIndex == 0)
	{
		minDepthUint = uint(-1);
		maxDepthUint = 0;
		visibleDecalsCount = 0;
		


	}
	
	GroupMemoryBarrierWithGroupSync();
 
	
	// Get thread depth
	int2 TextureSize;
	GBufferDepth.GetDimensions(TextureSize.x, TextureSize.y);
	const float2 Resolution = float2(TextureSize.x, TextureSize.y);
	const float2 TexelSize = 1 / Resolution;
	const float2 UV = TexelSize * (pixelPos + 0.5f);

	const float clipDepth = GBufferDepth[pixelPos].r;
	
	//float2 clipCoord = UV * 2.f - 1.f;
	//float4 clipPos = float4(clipCoord, clipDepth, 1.f);
	//clipPos.y *= -1.f; // Pixel coordinates(not NDC) for D3D start from upper left corner, so X stays the same but Y is flipped
	//const float4 worldPosHom = mul(clipPos, ConstBuffer.InvViewProj);
	//const float4 worldPos = worldPosHom / worldPosHom.w;

	//float linearizedDepth = ConstBuffer.Projection._43 / (clipDepth - ConstBuffer.Projection._33); 
	//uint depthUint = asuint(linearizedDepth);
	 
	uint depthUint = asuint(clipDepth);

	// Set depth min max for whole group

	InterlockedMin(minDepthUint, depthUint);
	InterlockedMax(maxDepthUint, depthUint);  

	GroupMemoryBarrierWithGroupSync();
	
	const uint2 TilesCount = ConstBuffer.NumWorkGroups;
	const uint2 TileIdx = GroupID.xy;


	const float maxDepth = asfloat(maxDepthUint);
	const float minDepth = asfloat(minDepthUint); 


	const float4x4 viewProj = mul(ConstBuffer.View, ConstBuffer.Projection);

	// Calculate frustum planes
	if (GroupIndex == 0)
	{
		// Multiplied by 2 simply because NDC for x,y goes from -1 to 1

		const float2 negativeStep = (2.f * float2(TileIdx)) / float2(TilesCount);

		const float2 positiveStepPlus1 = (2.f * float2(TileIdx + uint2(1, 1))) / float2(TilesCount);
		



		// Normal and distance
// Potential ones
// 
// //		// +x
//  		//frustumPlanes[0] = float4(1.f, 0.f, 0.f, -1 + negativeStep.x); // Wrong
// // 
// // 		// -x
// // 		frustumPlanes[1] = float4(-1.f, 0.f, 0.f, -1 + positiveStepPlus1.x); // Correct
// // 
// // 		// +y
// // 		frustumPlanes[2] = float4(0.f, 1.f, 0.f, 1 - negativeStep.y); // Correct
// // 
// // 		// -y
// // 		//frustumPlanes[3] = float4(0.f, -1.f, 0.f, 1 - positiveStepPlus1.y); // Wrong

		
// Valid ones
		// +x
		frustumPlanes[0] = float4(1.f, 0.f, 0.f, 1 - negativeStep.x);

		// TODO: Why are the wrong versions wrong? They seem like they should be correct on paper
		// Potentially could be that dot product between plane normal and distance can't be negative? That's one hypothesis that seems true
		// but maybe it's a side effect of something else

		// -x
		frustumPlanes[1] = float4(-1.f, 0.f, 0.f, -1 + positiveStepPlus1.x);

		// Y is different from OpenGl version and w is exchanged between +y and -y

		// +y
		frustumPlanes[2] = float4(0.f, 1.f, 0.f, -1 + positiveStepPlus1.y);

		// -y
		frustumPlanes[3] = float4(0.f, -1.f, 0.f, 1 - negativeStep.y);

		// Is there a reason to check near and far given that this will contain all decals because it's calculated based on them?
		// Yes, because this sets up the max and min depth for this Tile using the depth buffer, to compare with *all* decals

		// Near
		frustumPlanes[4] = float4(0.f, 0.f, 1.f, minDepth);
		// Far
		frustumPlanes[5] = float4(0.f, 0.f, -1.f, maxDepth);

		const float difference = 0.1f;

// Full screen test frustum
// 		// +x
// 		frustumPlanes[0] = float4(1.f, 0.f, 0.f, -1 + difference);
// 
// 		// -x
// 		frustumPlanes[1] = float4(-1.f, 0.f, 0.f, 1 - difference);
// 
// 		// Y is different from OpenGl version and w is exchanged between +y and -y
// 
// 		// +y
// 		frustumPlanes[2] = float4(0.f, 1.f, 0.f, -1 + difference);
// 
// 		// -y
// 		frustumPlanes[3] = float4(0.f, -1.f, 0.f, 1 - difference);




		for (int i = 0; i < 6; ++i)
			{
				// Planes are covariant
				// This means that it needs to be transformed by the transpose of the inverse of the matrix
				// https://math.stackexchange.com/questions/3123130/transforming-a-plane
				// https://stackoverflow.com/questions/7685495/transforming-a-3d-plane-using-a-4x4-matrix
				// This means that ViewProj can be used and instead of transposing, it can be post-multiplied(which is used in column major, not row-major)
				// to get the same behavior
				
				frustumPlanes[i] = mul(viewProj, frustumPlanes[i]);
				
				// Plane normalization
				frustumPlanes[i] = frustumPlanes[i] / length(frustumPlanes[i].xyz);
			}

	}
	
	GroupMemoryBarrierWithGroupSync();

	// Cull decals

	const uint threadCount = TILE_SIZE * TILE_SIZE;
	//const uint passCount = (ConstBuffer.NumDecals + threadCount - 1) / threadCount;
	const uint passCount = 1;

	for (uint i = 0; i < passCount; ++i)
	{
		const uint currDecalIdx = i * threadCount + GroupIndex;
		if (currDecalIdx >= ConstBuffer.NumDecals)
		{
			break;
		}

		const ShaderDecal currDecal = DecalBuffer[currDecalIdx];

		const float3 position = currDecal.Position;
		const float3x3 decalRot = transpose(QuatTo3x3(currDecal.Orientation));
		//const float3 rotatedPosition = mul(position, decalRot);

		bool visibleDecalInTile = true;
		// Check the planes of the frustum
		// If one fails, test fails
// Proper one
		float distance = 0.f;
		for (uint j = 0; j < 4; ++j)
		{
			distance = dot(position, frustumPlanes[j].xyz);

			//const float distanceOnPlaneNormal = abs(dot(frustumPlanes[j].xyz, currDecal.Size));

			//if (distance + ConstBuffer.DebugValue <= frustumPlanes[j].w )
			if (distance + ConstBuffer.DebugValue <= -frustumPlanes[j].w )
			{
				// Point is outside of plane
				visibleDecalInTile = false;
				break;
			}
		}

// Frustum test one
// 		const float2 positiveStepPlusHalf = (2.f * float2(float2(TileIdx) + float2(0.5f, 0.5f))) / float2(TilesCount);
// 		float2 frustumPoint = float2( -1 + positiveStepPlusHalf.x, 1 - positiveStepPlusHalf.y);
// 		//float3 frustumPointFull = float3(frustumPoint, clipDepth);
// 		float3 frustumPointFull = float3(frustumPoint, ConstBuffer.DebugValue);
// 		
// 		float4 frustumPointWorldPos = mul(float4(frustumPointFull, 1.f), ConstBuffer.InvViewProj);
// 		frustumPointWorldPos /= frustumPointWorldPos.w;
// 
// 
//  		float distance = 0.f;
// 		for (uint j = 0; j < 1; ++j)
// 		{
// 			//distance = dot(frustumPoint.xy, frustumPlanes[j].xy);
// 			distance = dot(frustumPointWorldPos.xyz, frustumPlanes[j].xyz);
// 
// 			//if (distance + ConstBuffer.DebugValue <= frustumPlanes[j].w )
// 			if (distance /*+ ConstBuffer.DebugValue */< frustumPlanes[j].w)
// 			{
// 				// Point is outside of plane
// 				visibleDecalInTile = false;
// 				break;
// 			}
// 		}




// Clip space test one
// 		float4 clipPos = mul(float4(position, 1.f), viewProj);
// 		clipPos /= clipPos.w;
// 		
// 		for (uint j = 0; j < 1; ++j)
// 		{
// 			distance = dot(clipPos.xyz, frustumPlanes[i].xyz);
// 	
// 			if (distance > frustumPlanes[i].w)
// 			{
// 				// Point is outside of plane
// 				visibleDecalInTile = false;
// 				break;
// 			}
// 		}


		// Debug transform frustum point to world space and compare against radius
// 		const float2 positiveStepPlusHalf = (2.f * float2(float2(TileIdx) + float2(0.5f, 0.5f))) / float2(TilesCount);
// 		float2 frustumPoint = float2( -1 + positiveStepPlusHalf.x, 1 - positiveStepPlusHalf.y);
// 		float3 frustumPointFull = float3(frustumPoint, clipDepth);
// 		
// 		float4 frustumPointWorldPos = mul(float4(frustumPointFull, 1.f), ConstBuffer.InvViewProj);
// 		frustumPointWorldPos /= frustumPointWorldPos.w;
// 
// 
// 		float4 clipPos = float4(position, 1.f);
// 		clipPos = mul(clipPos, viewProj);
// 		clipPos /= clipPos.w;
// 
// 		
// 		//float dist = length(frustumPoint - clipPos.xy);
// 		float dist = length(frustumPointWorldPos.xyz - position);
// 
// 		if (dist > 1.1f)
// 		{
// 			visibleDecalInTile = false;
// 		}


		if (visibleDecalInTile)
		{
			uint originalValue;
			InterlockedAdd(visibleDecalsCount, 1, originalValue);
			visibleDecalIndices[originalValue] = currDecalIdx;
		}
	}
	

	GroupMemoryBarrierWithGroupSync();
	
	
	if (GroupIndex == 0)
	{
		uint TileIdx = (GroupID.y * ConstBuffer.NumWorkGroups.x) + GroupID.x;
		uint address = (TileIdx * 4) + 1;

// 		if (ConstBuffer.DebugFlag == 0)
// 		{
// 			//TilingBuffer.Store(address, 0xFF);
// 			TilingBuffer.InterlockedOr(address, 0xFF);
// 		}
		
		if (visibleDecalsCount > 0)
		{
 			TilingBuffer.InterlockedOr(address, 0xFF);
		}
		

// 		for (uint i = 0; i < visibleDecalsCount; ++i)
// 		{
// 			uint mask = 0xFFFFFFFF;
// 
// 			TilingBuffer.InterlockedOr(address, mask);
// 		}

	}
	
	
		
	
	


}
