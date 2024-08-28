
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

Texture2D GBufferAlbedo : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferRoughness : register(t2);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;

    result.uv = uv;
    // Inverse y to simulate opengl like uv space
    result.uv.y = 1 - uv.y;

    result.position = position;    
    //result.position.x += offset;

    return result;
}

PSOutput PSMain(PSInput input)
{
    float2 uv = input.uv;

    PSOutput output;
    output.Color = GBufferAlbedo.Sample(g_sampler, uv);

    return output;
}
