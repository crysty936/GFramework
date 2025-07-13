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
    float SkyOnlyExposure;
};

// 256 byte aligned
ConstantBuffer<SkyboxConstantBuffer> SceneBuffer : register(b0);

struct CubemapIndexBuffer
{
    uint CubemapIdx;
};

// 256 byte aligned
ConstantBuffer<CubemapIndexBuffer> CubemapIdxContainer : register(b0, space1);

static const float FP16Scale = 0.0009765625f;
static const float FP16Max = 65000.0f;


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

#define TOLERANCE 0.1f

bool IsAroundOne(const float InVal)
{
    return abs(InVal) > (1.f - TOLERANCE) && abs(InVal) < (1.f + TOLERANCE);
    //return abs(InVal) == 1.f;
    //return abs(InVal) > .9f;
}

bool IsBoxEdge(const float3 InTexCoord)
{
    const bool xyAround1 = IsAroundOne(InTexCoord.x) && IsAroundOne(InTexCoord.y);
    const bool yzAround1 = IsAroundOne(InTexCoord.y) && IsAroundOne(InTexCoord.z);
    const bool xzAround1 = IsAroundOne(InTexCoord.x) && IsAroundOne(InTexCoord.z);

    const bool IsEdge = xyAround1 || xzAround1 || yzAround1;

    return IsEdge;
}


#define DISPLAY_CUBE_EDGES 0

PSOutput PSMain(PSInput input)
{
    PSOutput output;

#if DISPLAY_CUBE_EDGES
    const bool IsEdge = IsBoxEdge(input.CubemapUV);
    if (IsEdge)
    {
        output.Color = float4(0.f, 0.f, 0.f, 1.f);

        return output;
    }
#endif

    TextureCube cubeMap = ResourceDescriptorHeap[CubemapIdxContainer.CubemapIdx];
    float3 color = cubeMap.Sample(g_sampler, normalize(input.CubemapUV)).xyz;
    color *= exp2(SceneBuffer.SkyOnlyExposure);

    // reinhard tone mapping
    //color = color / (color + 1.f);

//     float2 uv = input.uv;
// 
//     PSOutput output;
//     float4 albedo = AlbedoMap.Sample(g_sampler, uv);
//     if (albedo.a == 0.f)
//     {
//         discard;
//     }

    //output.Color = float4(0.f, 1.f, 0.f, 1.f);

    //color = clamp(color, 0.f, FP16Max);
    output.Color = float4(color, 1.f);

    return output;
}
