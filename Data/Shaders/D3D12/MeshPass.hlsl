
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Roughness : SV_TARGET2;
};


// 256 byte aligned
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 Model;
    float4x4 Projection;
    float4x4 View;
    float padding[16];
};

Texture2D g_Albedo : register(t0);
Texture2D g_Normal : register(t1);
Texture2D g_Roughness : register(t2);

SamplerState g_sampler : register(s0);


PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD, float3 tangent : TANGENT, float3 bitangent : BITANGENT)
{
    PSInput result;

    result.uv = uv;
    // Inverse y to simulate opengl like uv space
    result.uv.y = 1 - uv.y;

    result.position = mul(Model, position);

    result.position = mul(View, result.position);
    result.position = mul(Projection, result.position);
    
    //result.position.x += offset;

    return result;
}

PSOutput PSMain(PSInput input)
{
    float2 uv = input.uv;

    PSOutput output;
    output.Albedo = g_Albedo.Sample(g_sampler, uv);
    output.Normal = g_Normal.Sample(g_sampler, uv);
    output.Roughness = g_Roughness.Sample(g_sampler, uv);

    return output;
}
