#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

layout(binding = 0) uniform sampler2D scene;
layout(binding = 1) uniform sampler2D bloomBlur;

void main()
{
	const float gamma = 2.2;
	vec3 hdrColor = texture(scene, ps_in.TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, ps_in.TexCoords).rgb;

	hdrColor += bloomColor; // additive blending

	// tone mapping
	//vec3 result = vec3(1.0) - exp(-hdrColor * exposure);

	// reinhard tone mapping
	vec3 result = hdrColor / (hdrColor + vec3(1.0));

	// also gamma correct while we're at it       
	result = pow(result, vec3(1.0 / gamma));
	FragColor = vec4(result, 1.0);
}