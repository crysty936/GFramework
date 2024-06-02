
struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

cbuffer ConstantBuffer
{
	float3 LightPos;
	float4x4 ScreenToWorld;
	float XRotation;
}

Texture2D TexDiffuse;
SamplerState TexDiffuse_sampler;

static const float3 WHITE = float3(1.000, 1.000, 1.000);
static const float3 YELLOW = float3(1.000, 0.843, 0.000);

// area reserved for ball number texture
static const float textureSquareSize = 0.28;
static const float textureEdge = 0.0033;

float4x4 rotationX(float angle)
{
	float4x4 rot = float4x4(
		1., 0., 0., 0.,
		0., cos(angle), -sin(angle), 0.,
		0., sin(angle), cos(angle), 0.,
		0., 0., 0., 1);
	
	return rot;
}

float GetCircleAlpha(PS_INPUT input)
{
	float2 circleCenter = float2(0.5, 0.5);

	float d = distance(circleCenter, input.TexCoord) * 2.0;

	float alpha = d > 0.5 ? 0.0 : 1.0;

	// smoothstep(min, max, x) returns inter between [0, 1] based on where x is in the given range 
	float alpha2 = d > 0.99 ? 1.0 - smoothstep(0.99, 1.0, d) : 1.0;

	return alpha2;
}

float3 GetNormal(PS_INPUT input)
{
	float2 uv = input.TexCoord;
	// Transform coordinates to -1..1 range
	uv = (uv * 2.0) - 1.0;

	// dot(uv, uv) == uv.x^2 - uv.y^2
	float3 normal = float3(uv.xy, sqrt(1.0 - clamp(dot(uv, uv), 0.0, 1.0)));

	return normal;
}

float3 GetColor(PS_INPUT input, float3 inNormal)
{
	float3 color = YELLOW;

	[branch]
	if (abs(inNormal.y) > 0.55)
	{
		float weight = smoothstep(0.55, 0.56, abs(inNormal.y));

		//return lerp(YELLOW, WHITE, weight); // TODO: Why X4000 error?
		color = lerp(YELLOW, WHITE, weight);
	}
	else
	{
		float d = distance(abs(inNormal.xy), float2(0.0, 0.0));

		[branch]
		if (d < textureSquareSize)
		{
			// Convert coordinates to texture coordinates and increase the sampling area a bit
			float2 tex = (inNormal.xy + textureSquareSize) / ((textureSquareSize * 2) - 0.025);

			// These xy are modified from before from range 0..1 to -1..1 so 0 is middle here - this does not work because xy can have different signs
			//float2 tex = smoothstep(0.0, textureSquareSize, abs(inNormal.xy));

			// This has to be done in OpenGL because different UV starting points
			//tex.y = 1.0 - tex.y;

			tex = tex / 4.0;

			float ballNumber = 10;

			tex.x = tex.x + fmod(ballNumber, 4.0) * 0.25;
			tex.y = 0.75 - (int(ballNumber / 4.0) * 0.25) + tex.y;

			float3 texColor = TexDiffuse.Sample(TexDiffuse_sampler, tex).xyz;

			color = texColor;
		}
		else if (d < textureSquareSize + 0.01) // Smooth out circle edge
		{
			float weight = smoothstep(textureSquareSize, textureSquareSize + 0.01, d);
			color = lerp(WHITE, YELLOW, weight);
		}
	}

	return color;
}

float4 PSEntry(PS_INPUT input) : SV_Target
{
	float alpha = GetCircleAlpha(input);
	float3 normal = GetNormal(input);
	
	float angle = XRotation;
	
	float4 vert = float4(normal.x, normal.y, normal.z, 0.0);

	float4x4 rot = rotationX(angle);



	float brightness = clamp(dot(normalize(LightPos), vert.xyz), 0.1, 1.0);
	//float brightness = 1.0;

	// Test normals
	//float3 visualizedNormal = 0.5 + 0.5 * normal;
	//float3 color = visualizedNormal;

	// Flat color
	//float3 color = yellow;

	// Test texture
	//float3 color = TexDiffuse.Sample(TexDiffuse_sampler, input.TexCoord).xyz;

	vert = mul(rot, vert);
	float3 color = GetColor(input, vert.xyz);
	float4 finalColor = float4(brightness * color, alpha);

    return finalColor;
}
