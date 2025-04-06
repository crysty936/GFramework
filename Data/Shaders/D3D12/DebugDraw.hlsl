#include "Utils.hlsl"
#include "Packing.hlsl"

// VetexID = 0, 1, 2, 2, 3, 0
static const float4 Quad[] =
{
    float4(1.f, -1.f, 0.0f, 1.0f),  // bottom right
    float4(1.f,  1.f, 0.0f, 1.0f),  // top right
    float4(-1.f,  1.f, 0.0f, 1.0f), // top left 
    float4(-1.f, -1.f, 0.0f, 1.0f),  // bottom left
};


struct InterpolantsVSToPS
{
    float4 Position : SV_POSITION;
    nointerpolation float4 Color : COLOR;
};

// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 Projection;
    float4x4 View;
};

struct PackedDebugPointInstanceData
{
	float3 Translation;
    float Scale;
	uint Color;
	uint Padding[3];
};

// 256 byte aligned
ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);
// 16 byte aligned
StructuredBuffer<PackedDebugPointInstanceData> PointsBuffer : register(t0);

InterpolantsVSToPS VSMainPoints(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    float4 position = Quad[vertexId];
    const PackedDebugPointInstanceData instanceData = PointsBuffer[instanceId];
    position.xyz = (position.xyz * instanceData.Scale) + instanceData.Translation;
    
    const float4x4 vp = mul(SceneBuffer.View, SceneBuffer.Projection);
    const float4 clipPos = mul(position, vp);

    InterpolantsVSToPS result;
    result.Position = clipPos;
    result.Color = RGBA8_UNORM::Unpack(instanceData.Color);

    return result;
}

float4 PSMainPoints(InterpolantsVSToPS input) : SV_Target0
{
    return input.Color;
}
