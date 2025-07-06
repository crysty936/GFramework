
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
	unsigned int TextureType;
	float CameraNear;
	float CameraFar;

	float Padding[48];
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

	
	switch (ConstBuffer.TextureType)
	{
		// Standard
		case 0:
		{
			float4 sample = TextureToDebug.Sample(g_sampler, uv);
			const float gamma = 2.2;
			const float inverseGamma = 1.0 / gamma;
			// gamma correction 
			sample = float4(pow(sample.xyz, float3(inverseGamma, inverseGamma, inverseGamma)), sample.a);
			output.Color = sample;

			break;
		}

		// Render target
		case 1:
		{
			const float4 sample = TextureToDebug.Sample(g_sampler, uv);
			output.Color = sample;

			break;
		}

		// Depth target
		case 2:
		{
			const float depth = TextureToDebug.Sample(g_sampler, uv).r;

			// Transform depth into view space
			// Basically inverse of projection, only applied to z
			float linearizedDepth = (ConstBuffer.CameraNear * ConstBuffer.CameraFar) / (ConstBuffer.CameraFar +  depth * (ConstBuffer.CameraNear - ConstBuffer.CameraFar));
			linearizedDepth /= (ConstBuffer.CameraFar);

			output.Color = float4(linearizedDepth, linearizedDepth, linearizedDepth, 1.f);

			// Same as above, mathematically derived pre-projection z out of 
			// the operations that happen with z when multiplied with the projection
			//float3 linearizedDepth = ConstBuffer.Proj._43 / (depth - ConstBuffer.Proj._33); 
			//linearizedDepth = linearizedDepth / 50;

			break;
		}
		

	}

    return output;

}
