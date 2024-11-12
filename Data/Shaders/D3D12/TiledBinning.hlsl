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
struct DecalTilingConstantBuffer
{
    float4x4 Projection;
    float4x4 View;
	float4x4 InvViewProj;
	uint NumDecals;
	uint2 NumWorkGroups;
	uint DebugFlag;
	float4 DebugValue;
	float4 DebugQuat;

	float Padding[4];
};

// 256 byte aligned
ConstantBuffer<DecalTilingConstantBuffer> ConstBuffer : register(b0);

SamplerState g_sampler : register(s0);

RWByteAddressBuffer TilingBuffer : register(u0);

RWTexture2D<float4> OutputDebug : register(u1);

groupshared uint minDepthUint;
groupshared uint maxDepthUint;
groupshared uint visibleDecalsCount;
groupshared float4 frustumPlanes[6];
groupshared float4 frustumPlanesOrigins[6];
groupshared uint visibleDecalIndices[1024];
groupshared uint visibleDecalIndicesWord; // TODO: Have to make based on the number of 32 bits [n]

static const float3 BoxPoints[8] =
{
	float3(1.0f, 1.0f, -1.f),
	float3(-1.0f, 1.0f, -1.f),
	float3(1.0f, -1.0f, -1.f),
	float3(-1.0f, -1.0f, -1.f),
	float3(1.0f, 1.0f, 1.f),
	float3(-1.0f, 1.0f, 1.f),
	float3(1.0f, -1.0f, 1.f),
	float3(-1.0f, -1.0f, 1.f)
};

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
		visibleDecalIndicesWord = 0;
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
// Logical ones
// These are correct.
// It is just because of how the comparison is done at the end with w, that the w for some needs to be negated.
// Otherwise, difference comparison operations would be needed for different planes
// Using the logical w and negating the other one ensures that the same comparison operation can be used for both
// Same thing would be valid with a different comparison operation and negating the other side
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

		// +x
		frustumPlanes[0] = float4(1.f, 0.f, 0.f, 1 - negativeStep.x);

		// -x
		frustumPlanes[1] = float4(-1.f, 0.f, 0.f, -1 + positiveStepPlus1.x);

		// Y is different from OpenGl version and w is exchanged between +y and -y

		// +y
		frustumPlanes[2] = float4(0.f, 1.f, 0.f, -1 + positiveStepPlus1.y);

		// -y
		frustumPlanes[3] = float4(0.f, -1.f, 0.f, 1 - negativeStep.y);

		// Also compare depth, to not process decal pixels that are behind visible geometry

		// Near
		frustumPlanes[4] = float4(0.f, 0.f, 1.f, minDepth);
		// Far
		frustumPlanes[5] = float4(0.f, 0.f, -1.f, maxDepth);

	}

	GroupMemoryBarrierWithGroupSync();

// 	if(GroupIndex == 0)
// 	{
// // 		for (int i = 0; i < 6; ++i)
// // 			{
// // 				// Planes are covariant
// // 				// This means that it needs to be transformed by the transpose of the inverse of the matrix
// // 				// https://math.stackexchange.com/questions/3123130/transforming-a-plane
// // 				// https://stackoverflow.com/questions/7685495/transforming-a-3d-plane-using-a-4x4-matrix
// // 				// This means that ViewProj can be used and instead of transposing, it can be post-multiplied(which is used in column major, not row-major)
// // 				// to get the same behavior
// // 				
// // 				frustumPlanes[i] = mul(viewProj, frustumPlanes[i]);
// // 				
// // 				// Plane normalization
// // 				frustumPlanes[i] = frustumPlanes[i] / length(frustumPlanes[i].xyz);
// // 			}
// 	}
	
	GroupMemoryBarrierWithGroupSync();

	// Cull decals

	const uint threadCount = TILE_SIZE * TILE_SIZE;
	const uint passCount = (ConstBuffer.NumDecals + threadCount - 1) / threadCount;
	//const uint passCount = 1;


	// Have to be done outside of pass because inside only the threads that can pick up a decal will write
	//OutputDebug[pixelPos] = frustumPlanes[1];

	for (uint passIdx = 0; passIdx < passCount; ++passIdx)
	{
		const uint currDecalIdx = passIdx * threadCount + GroupIndex;
		if (currDecalIdx >= ConstBuffer.NumDecals)
		{
			break;
		}
		//const uint currDecalIdx = 0;

		const ShaderDecal currDecal = DecalBuffer[currDecalIdx];

		const float3 decalPos = currDecal.Position;
		const float3x3 decalRot = QuatTo3x3(currDecal.Orientation);

		//const float3x3 inverseDecalRot = transpose(decalRot);
		//const float3x3 debugRot = QuatTo3x3(ConstBuffer.DebugQuat);

		//bool visibleDecalInTile = true;
 		float distance = 0.f;
		// Check the planes of the frustum
		// If one fails, test fails

// World Space test one. Uses radius test so not correct
// 		for (uint j = 1; j < 2; ++j)
// 		{
// 			//float3 currPlane = frustumPlanes[j].xyz;
// 			//currPlane = mul(currPlane.xyz, decalRot);
// 			//distance = dot(position, currPlane);
// 
// 			float3 frustumPlaneNormal = frustumPlanes[j].xyz;
// 			float3 frustumPlaneOrigin = frustumPlanesOrigins[j].xyz;
// 			
// 			//frustumPlaneNormal = mul(frustumPlaneNormal, decalRot);
// 			//frustumPlaneNormal = mul(frustumPlaneNormal, debugRot);
// 			//frustumPlaneNormal = normalize(frustumPlaneNormal);
// 			
// 
// 			distance = dot(position, frustumPlaneNormal);
// 			float originDistance = dot(frustumPlaneNormal, frustumPlaneOrigin);
// 
// 
// 			//const float distanceOnPlaneNormal = abs(dot(frustumPlanes[j].xyz, currDecal.Size));
// 
// 			if (distance <= originDistance )
// 			//if (distance <= frustumPlanes[j].w )
// 			//if (distance + ConstBuffer.DebugValue.x <= frustumPlanes[j].w )
// 			//if (distance + ConstBuffer.DebugValue.x <= -frustumPlanes[j].w )
// 			{
// 				// Point is outside of plane
// 				visibleDecalInTile = false;
// 				break;
// 			}
//		}


//TODO: Use a border/ortho transform at the end that 
// transforms relative to this compute group's position and size and just check against that based on if the points are outside of the bounds(-1, 1), 
// instead of checking all points for all planes

		float3 clipPositions[8];

		// Transform decal corner positions to clip space
		for (uint posIdx = 0; posIdx < 8; ++posIdx)
		{
			float3 currPoint = BoxPoints[posIdx];
			// TODO: Better to send over matrix and multiply than this
			currPoint = mul(currPoint, decalRot); // Rotate
			currPoint = currPoint + decalPos; // Translate	
			currPoint = currPoint * currDecal.Size; // Scale
			
			float4 clipSpacePoint = float4(currPoint, 1.f);
			clipSpacePoint = mul(clipSpacePoint, viewProj);
			clipSpacePoint /= clipSpacePoint.w;
			
			clipPositions[posIdx] = clipSpacePoint.xyz;
		}



		bool visibleDecalInTile = false;

		// Output clip positions debug
// 		const float2 positiveStepPlusHalf = (2.f * float2(float2(TileIdx) + float2(0.5f, 0.5f))) / float2(TilesCount);
// 		const float2 frustumPoint = float2( -1 + positiveStepPlusHalf.x, 1 - positiveStepPlusHalf.y);		
// 		for (uint posIdx2 = 0; posIdx2 < 8; ++posIdx2)
// 		{	
// 			const float3 currPos = clipPositions[posIdx2];
// 			const float distance = length(currPos.xy - frustumPoint);
// 			if (distance < 0.05f)
// 			{
// 				visibleDecalInTile = true;
// 			}
// 		}

		bool allPlanesValid = true;
		for (uint planeIdx = 0; planeIdx < 6; ++planeIdx)
		{
			bool visiblePosInTile = false;
			for (uint posIdx = 0; posIdx < 8; ++posIdx)
			{
				const float3 currPos = clipPositions[posIdx];
				distance = dot(currPos, frustumPlanes[planeIdx].xyz);
				const bool test = distance > -frustumPlanes[planeIdx].w;
				OutputDebug[pixelPos] = float4(frustumPlanes[planeIdx].w, distance, float(test), 0.f);

				if (test)
				{
					// Point is inside plane
					visiblePosInTile = true;
					break;
				}
			}

			allPlanesValid &= visiblePosInTile;
 		}

		visibleDecalInTile = allPlanesValid;

		if (visibleDecalInTile)
		{
			//uint originalValue;
			//InterlockedAdd(visibleDecalsCount, 1, originalValue);
			//visibleDecalIndices[originalValue] = currDecalIdx;
			
			InterlockedOr(visibleDecalIndicesWord, 1u << currDecalIdx);
			//visibleDecalIndicesWord &= ;
		}
	}
	

	GroupMemoryBarrierWithGroupSync();
	

	OutputDebug[pixelPos] = float4(float(visibleDecalIndicesWord), 0.f, 0.f, 0.f);
	
	if (GroupIndex == 0)
	{
		uint TileIdx = (GroupID.y * ConstBuffer.NumWorkGroups.x) + GroupID.x;
		uint address = (TileIdx * 4) + 1;
		
		TilingBuffer.InterlockedOr(address, visibleDecalIndicesWord);
	}

}
