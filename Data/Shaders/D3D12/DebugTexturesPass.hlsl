
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

// 256 byte aligned
struct DebugTexturesConstantBuffer
{
	float4 TranslationScale;
	//float4x4 ProjInv;
	//float4x4 Proj;

    //float4 ViewPos;
    //float4 LightDir;
    float Padding[51];
};

ConstantBuffer<DebugTexturesConstantBuffer> ConstBuffer : register(b0);

Texture2D TextureToDebug : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;

    result.uv = uv;
    // Inverse y to simulate opengl like uv space
    result.uv.y = 1 - uv.y;
	float4 finalPos = position;
	finalPos *= float4(ConstBuffer.TranslationScale.zw, 1.f, 1.f);
	finalPos += float4(ConstBuffer.TranslationScale.xy, 0.f, 0.f);

    result.position = finalPos;    

    return result;
}

PSOutput PSMain(PSInput input)
{
    const float2 uv = input.uv;

    PSOutput output;
    //float4 albedo = GBufferAlbedo.Sample(g_sampler, uv);

	//float depth = GBufferDepth.Sample(g_sampler, uv).r;

	const float cameraNear = 0.1f;
	const float cameraFar = 5.f;
	// Transform depth into view space
	// Basically inverse of projection, only applied to z
	//float3 linearizedDepth = (cameraNear * cameraFar) / (cameraFar +  depth * (cameraNear - cameraFar));

	// Same as above, mathematically derived pre-projection z out of 
	// the operations that happen with z when multiplied with the projection
	//float3 linearizedDepth = ConstBuffer.Proj._43 / (depth - ConstBuffer.Proj._33); 
	//linearizedDepth = linearizedDepth / 50;

	//output.Color = float4(linearizedDepth, 1.f);
	//output.Color = albedo;
	//output.Color = float4(wsNormal0to1, 1.f);
	output.Color = float4(1.f, 0.f, 0.f, 1.f);

    return output;

}
