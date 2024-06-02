#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

uniform sampler2D quadTexture;

layout(std140, binding = 0) uniform GaussianParamsBuffer
{
	int Horizontal;
} ParamsBuffer;

float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
	vec2 tex_offset = 1.0 / textureSize(quadTexture, 0); // gets size of single texel
	vec3 result = texture(quadTexture, ps_in.TexCoords).rgb * weight[0]; // current fragment's contribution
	if (bool(ParamsBuffer.Horizontal))
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(quadTexture, ps_in.TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(quadTexture, ps_in.TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	}
	else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(quadTexture, ps_in.TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(quadTexture, ps_in.TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}
	}

	FragColor = vec4(result, 1.0);
}