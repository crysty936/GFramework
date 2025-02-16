#include "DescriptorTables.hlsl"
#include "Utils.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float3 CubemapUV : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

// 256 byte aligned
struct SkyboxConstantBuffer
{
    float4x4 Projection;
    float4x4 View;
};

// 256 byte aligned
ConstantBuffer<SkyboxConstantBuffer> SceneBuffer : register(b0);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION)
{
    // Use just rotation of View Matrix
    float4 clipPos = mul(float4(position.xyz, 0.f), SceneBuffer.View);

    // Project to clip space
    clipPos = mul(float4(clipPos.xyz, 1.f), SceneBuffer.Projection);
    
    // Make sure that this is always at the far plane after the divide
    PSInput result;
    result.position = clipPos.xyww;
    result.CubemapUV = position.xyz;

    return result;
}

PSOutput PSMain(PSInput input)
{
//     float2 uv = input.uv;
// 
//     PSOutput output;
//     float4 albedo = AlbedoMap.Sample(g_sampler, uv);
//     if (albedo.a == 0.f)
//     {
//         discard;
//     }

    PSOutput output;
    output.Color = float4(1.f, 0.f, 0.f, 1.f);

    return output;
}
