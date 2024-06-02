#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

uniform sampler2D quadTexture;

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

	vec4 diffuse = texture(quadTexture, ps_in.TexCoords);
	vec3 color = pow(diffuse.xyz, vec3(1.0 / 2.2));
	FragColor = vec4(color.xyz, diffuse.w);
}