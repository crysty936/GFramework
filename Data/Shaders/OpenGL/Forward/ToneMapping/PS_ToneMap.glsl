#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

layout(std140, binding = 0) uniform ExposureBuffer
{
	float Exposure;
} exposureBuffer;

uniform sampler2D quadTexture;

void main()
{
	const float gamma = 2.2;
	vec3 hdrColor = texture(quadTexture, ps_in.TexCoords).xyz;

	// exposure tone mapping
	//vec3 mapped = vec3(1.0) - exp(-hdrColor * exposureBuffer.Exposure);

	// reinhard tone mapping
	vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

	// gamma correction 
	mapped = pow(mapped, vec3(1.0 / gamma));

	FragColor = vec4(mapped, 1.0);
}