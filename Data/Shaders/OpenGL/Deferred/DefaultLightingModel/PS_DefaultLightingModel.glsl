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

layout(std140, binding = 2) uniform LightingDataBuffer
{
	int bUseDirLight;
	vec3 DirectionalLightDir;
	vec3 ViewPos;
} LightingConstants;

layout(binding = 0) uniform sampler2D GBufferAlbedoSpec;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferDepth;

vec3 CalcDirLight(vec3 lightDir, vec3 normal, vec3 viewDir)
{
	vec4 albedoSpec = texture(GBufferAlbedoSpec, ps_in.TexCoords);
	vec3 ambient = 0.2 * albedoSpec.xyz;

	// diffuse shading
	float diff = clamp(dot(normal, -lightDir), 0.0, 1.0);
	vec3 diffuse = diff * albedoSpec.xyz;

	// specular shading
	// Blinn-Phong
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
	vec3 specular = spec * vec3(albedoSpec.a, albedoSpec.a, albedoSpec.a);

	return (ambient + diffuse + specular);
}


void main()
{
	const float depthValue = texture(GBufferDepth, ps_in.TexCoords).r;
	FragColor = vec4(vec3(depthValue), 1.0);

	vec3 wsNormal = texture(GBufferNormal, ps_in.TexCoords).rgb;
	wsNormal = wsNormal * 2.0 - 1.0;
	//FragColor = vec4(wsNormal, 1.0);

	vec2 clipCoord = ps_in.TexCoords.xy * 2.0 - 1.0;
	vec4 clipsSpaceLoc = vec4(clipCoord.x, clipCoord.y, depthValue, 1.0);

	vec4 viewSpacePosition = ViewConstants.perspInv * clipsSpaceLoc;
	viewSpacePosition /= viewSpacePosition.w;
	vec4 worldSpacePos = ViewConstants.viewInv * viewSpacePosition;
	//vec3 albedo = texture(GBufferAlbedoSpec, ps_in.TexCoords).rgb;
	//FragColor = vec4(albedo, 1.0);

	vec3 viewToFragW = normalize(worldSpacePos.xyz - LightingConstants.ViewPos);
	FragColor = vec4(CalcDirLight(LightingConstants.DirectionalLightDir, wsNormal, viewToFragW), 1.0);
}