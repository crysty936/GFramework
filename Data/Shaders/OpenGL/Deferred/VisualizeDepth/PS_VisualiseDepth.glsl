#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

layout(std140, binding = 1) uniform ViewConstantsBuffer
{
	mat4 perspInv;
	mat4 viewInv;
	vec4 CameraNearFar;
	vec4 ViewportSize;

} ViewConstants;


uniform sampler2D quadTexture;

void main()
{
	if (ps_in.TexCoords.x > 0.99f || ps_in.TexCoords.x < 0.01f || ps_in.TexCoords.y > 0.99f || ps_in.TexCoords.y < 0.01f)
	{
		FragColor = vec4(0.f, 1.f, 0.f, 1.f);
		return;
	}

	float depthValue = texture(quadTexture, ps_in.TexCoords).r;

	vec2 clipCoord = ps_in.TexCoords.xy * 2.f - 1.f;

	// Only works if quad is fullscreen
	//vec2 clipCoord = (gl_FragCoord.xy / ViewConstants.ViewportSize.xy) * 2.f - 1.f;

	vec4 clipsSpaceLoc = vec4(clipCoord.x, clipCoord.y, depthValue, 1.0);

	vec4 viewSpacePos = ViewConstants.perspInv * clipsSpaceLoc;
	viewSpacePos /= viewSpacePos.w;

 	float far = ViewConstants.CameraNearFar.y;
 	float normalizedDepth = abs(viewSpacePos.z) / far;
	FragColor = vec4(vec3(normalizedDepth), 1.0);
 
	if (depthValue > 0.999999f)
	{
		FragColor = vec4(1.f, 1.f, 1.f, 1.f);
	}
}