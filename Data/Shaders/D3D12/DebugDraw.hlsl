#include "Utils.hlsl"
#include "Packing.hlsl"

// VetexID = 0, 1, 2, 2, 3, 0
static const float4 Quad[] =
{
    float4(1.f, -1.f, 0.0f, 1.0f),  // bottom right
    float4(1.f,  1.f, 0.0f, 1.0f),  // top right
    float4(-1.f,  1.f, 0.0f, 1.0f), // top left 
    float4(-1.f, -1.f, 0.0f, 1.0f), // bottom left
};


struct InterpolantsVSToPS
{
    float4 Position : SV_POSITION;
    nointerpolation float4 Color : COLOR;
};

// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 WorldToClip;
    float4x4 CameraToWorld;
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
    const float4 localPosition = Quad[vertexId];
    const PackedDebugPointInstanceData instanceData = PointsBuffer[instanceId];
    
    // World space origin oriented
    //const float3 worldPosition = (localPosition.xyz * instanceData.Scale) + instanceData.Translation;

    // Camera aligned    
    const float4 cameraRight = mul(float4(1.f, 0.f, 0.f, 0.f), SceneBuffer.CameraToWorld);
    const float4 cameraUp = mul(float4(0.f, 1.f, 0.f, 0.f), SceneBuffer.CameraToWorld);
    const float3 worldPosition = ((cameraRight.xyz * localPosition.x) + (cameraUp.xyz * localPosition.y)) * instanceData.Scale + instanceData.Translation;

    const float4 clipPos = mul(float4(worldPosition, 1.f), SceneBuffer.WorldToClip);

    InterpolantsVSToPS result;
    result.Position = clipPos;
    result.Color = RGBA8_UNORM::Unpack(instanceData.Color);

    return result;
}

float4 PSMainPoints(InterpolantsVSToPS input) : SV_Target0
{
    return input.Color;
}


struct PackedDebugLineInstanceData
{
	float3 Position[2];
	uint Color;
	uint Padding;
};

StructuredBuffer<PackedDebugLineInstanceData> LinesBuffer : register(t0);

InterpolantsVSToPS VSMainLines(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    const PackedDebugLineInstanceData instanceData = LinesBuffer[instanceId];
    const float4 position = float4(instanceData.Position[vertexId], 1.f);

    const float4 clipPos = mul(position, SceneBuffer.WorldToClip);

    InterpolantsVSToPS result;
    result.Position = clipPos;
    result.Color = RGBA8_UNORM::Unpack(instanceData.Color);

    return result;
}

float4 PSMainLines  (InterpolantsVSToPS input) : SV_Target0
{
    return input.Color;
}
