
cbuffer ConstantBuffer
{
	matrix Projection;
	matrix View;
	matrix Model;
}

struct VS_INPUT
{
	float4 Pos : POSITION;
	float2 Coord : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

 
PS_INPUT VSEntry(VS_INPUT input)
{
	//float4 output = mul(input.Pos, Model);
	//output = mul(output, View);
	//output = mul(output, Projection);

	float4 output = mul(Model, input.Pos);
	output = mul(View, output);
	output = mul(Projection, output);

	PS_INPUT psOut;
	psOut.Pos = output;
	psOut.TexCoord = input.Coord;

    return psOut;
}
