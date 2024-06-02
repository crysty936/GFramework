#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

layout(binding = 0) uniform sampler2D quadTexture;

//uniform float near = 0.1;
//uniform float far = 1000.0;

// For drawing from a texture that contains depth only
//float LinearizeDepth(float inDepth)
//{
//	float z = inDepth * 2.0 - 1.0;
//	float farExtracted = (2.0 * near * far) / (far + near - z * (far - near));
//
//	return farExtracted;
//}
//
//
//float fragmentDepth = LinearizeDepth(gl_FragCoord.z) / far;


void main()
{
	//float depthValue = texture(quadTexture, inTexCoords).r;
	//FragColor = vec4(vec3(depthValue), 1.0);

	if (ps_in.TexCoords.x > 0.995f || ps_in.TexCoords.x < 0.005f || ps_in.TexCoords.y > 0.995f || ps_in.TexCoords.y < 0.005f)
	{
		FragColor = vec4(0.f, 1.f, 0.f, 1.f);
		return;
	}

	vec4 color = texture(quadTexture, ps_in.TexCoords);
	FragColor = vec4(color.x, color.y, color.z, 1.0);
}