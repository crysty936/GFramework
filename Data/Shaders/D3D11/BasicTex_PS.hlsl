
Texture2D TexDiffuse;
SamplerState TexDiffuse_sampler;

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

float4 SampleDiffuse(PS_INPUT input)
{
	//uint uWidth, uHeight;
	//TexDiffuse.GetDimensions(uWidth, uHeight);

	//float2 invSize = float2(1.0 / uWidth, 1.0 / uHeight);

	//float2 relPos = input.Pos.xy * invSize;
	//bool isEdge = any(abs(relPos - 0.5) > 0.48);


	float2 UVs = input.TexCoord;
	bool isEdge = abs(UVs - 0.5).x > 0.48 || abs(UVs - 0.5).y > 0.48;

	float4 color = float4(0.5, 0.5, 0.5, 1.0);

	[branch]
	if (!isEdge)
	{
		color = TexDiffuse.Sample(TexDiffuse_sampler, UVs);
	}

	return color;
}

float4 PSEntry(PS_INPUT input) : SV_Target
{
	float4 color = SampleDiffuse(input);
	
	return color;
}
