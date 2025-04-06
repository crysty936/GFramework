#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

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

struct DebugPointInstanceData
{
	float4x4 Model;
	float4 Color;
};

// 256 byte aligned
ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);
// 16 byte aligned
StructuredBuffer<DebugPointInstanceData> PointsBuffer : register(t0);

InterpolantsVSToPS VSMainPoints(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    float4 position = Quad[vertexId];
    DebugPointInstanceData instanceData = PointsBuffer[instanceId];
    
    //const float4 worldPos = mul(position, SceneBuffer.Model);
    //const float4 clipPos = mul(mul(worldPos, SceneBuffer.View), SceneBuffer.Projection);

    const float4x4 mv = mul(instanceData.Model, SceneBuffer.View);
    const float4x4 mvp = mul(mv, SceneBuffer.Projection);
    const float4 clipPos = mul(position, mvp);

    InterpolantsVSToPS result;
    result.Position = clipPos;
    result.Color = instanceData.Color;

    return result;
}

float4 PSMainPoints(InterpolantsVSToPS input) : SV_Target0
{
    return input.Color;
}
