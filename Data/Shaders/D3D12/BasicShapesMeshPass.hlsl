#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 VertexNormalWS : VERTEX_NORMAL;
};

struct PSOutput
{
    float4 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Roughness : SV_TARGET2;
};


// 256 byte aligned
struct SceneConstantBuffer
{
    float4x4 Model;
    float4x4 Projection;
    float4x4 View;
    float4x4 LocalToWorldRotationOnly;
};

struct MatIndexBuffer
{
    uint Index;
};

// 256 byte aligned
ConstantBuffer<SceneConstantBuffer> SceneBuffer : register(b0);
ConstantBuffer<MatIndexBuffer> MatIndex : register(b0);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    const float4 worldPos = mul(position, SceneBuffer.Model);
    const float4 clipPos = mul(mul(worldPos, SceneBuffer.View), SceneBuffer.Projection);

     const float3x3 LocalToWorldRotationOnly3x3 = ToFloat3x3(SceneBuffer.LocalToWorldRotationOnly);
     float3 vertexNormalWS = normalize(mul(normal, LocalToWorldRotationOnly3x3)); 

    result.uv = uv;
    // Inverse y to simulate opengl like uv space
    result.uv.y = 1 - uv.y;

    result.position = clipPos;
    result.VertexNormalWS = vertexNormalWS;

    return result;
}

PSOutput PSMain(PSInput input)
{
    float2 uv = input.uv;

    PSOutput output;
    //output.Albedo = g_texture.Sample(g_sampler, uv);

    Texture2D AlbedoMap = Tex2DTable[MatIndex.Index];
    output.Albedo = AlbedoMap.Sample(g_sampler, uv);

    output.Normal = float4(input.VertexNormalWS / 2.f + 0.5f, 1.0);
    output.Roughness = float4(1.f, 0.f, 0.f, 0.f);

    return output;
}
