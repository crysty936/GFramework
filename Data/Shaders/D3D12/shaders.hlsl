
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
    float4 Color2 : SV_TARGET1;
};


// 256 byte aligned
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 Model;
    float4x4 Projection;
    float4x4 View;
    float padding[16];
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);


PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
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
    PSOutput output;
    output.Color = g_texture.Sample(g_sampler, input.uv);
    output.Color2 = float4(1.f, 0.f, 0.f, 1.f);

    return output;
}
