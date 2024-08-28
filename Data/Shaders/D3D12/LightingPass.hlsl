
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};


Texture2D g_texture : register(t0);
Texture2D g_texture2 : register(t1);
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
    output.Color = g_texture.Sample(g_sampler, uv);
    output.Color += g_texture2.Sample(g_sampler, uv).g;


    return output;
}
