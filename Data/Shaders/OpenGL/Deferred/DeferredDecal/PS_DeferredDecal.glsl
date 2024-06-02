#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedoSpec;

in VS_OUT
{
	mat4 clipToWorldMatrix;
} ps_in;

layout(std140, binding = 1) uniform ViewConstantsBuffer
{
	mat4 perspInv;
	mat4 viewInv;
	vec4 CameraNearFar;
	vec4 ViewportSize;

} ViewConstants;

layout(binding = 0) uniform sampler2D Diffuse;
layout(binding = 1) uniform sampler2D GBufferDepth;

void main()
{
	vec2 uvs = (gl_FragCoord.xy / ViewConstants.ViewportSize.xy);

	const float depthValue = texture(GBufferDepth, uvs).r;

	vec2 clipCoord = (gl_FragCoord.xy / ViewConstants.ViewportSize.xy) * 2.f - 1.f;
	vec4 clipsSpaceLoc = vec4(clipCoord.x, clipCoord.y, depthValue, 1.0);

	vec4 viewSpacePos = ViewConstants.perspInv * clipsSpaceLoc;
	viewSpacePos /= viewSpacePos.w;

	//float normalizedDepth = abs(viewSpacePos.z) / ViewConstants.CameraNearFar.y;
	//gAlbedoSpec = vec4(vec3(normalizedDepth), 1.0);

	vec3 svPosition = vec3(clipCoord.x, clipCoord.y, depthValue);

	vec4 osPosition = ps_in.clipToWorldMatrix * vec4(svPosition, 1.0);
	osPosition = osPosition / osPosition.w;
	
	// Discard pixels with outside the cube bounds

	if (any(lessThan(osPosition.xyz + vec3(1.0f), vec3(0.f))))
	{
		discard;
	}

	if (any(lessThan(vec3(1.0f) - osPosition.xyz, vec3(0.f))))
	{
		discard;
	}

	vec3 DecalVector = osPosition.xyz * 0.5f + 0.5f;

	//// Swizzle so that DecalVector.xy are perpendicular to the projection direction and DecalVector.z is distance along the projection direction
	vec3 SwizzlePos = DecalVector.zxy;

	//// By default, map textures using the vectors perpendicular to the projection direction
	vec2 DecalUVs = SwizzlePos.xy;

	vec3 color = texture(Diffuse, DecalUVs).rgb;

	gAlbedoSpec = vec4(color, 1.0);
}