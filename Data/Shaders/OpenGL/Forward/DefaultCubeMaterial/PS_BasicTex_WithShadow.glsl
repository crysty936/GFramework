#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 Normal;
	vec3 DirectionalLightDirection;
	mat4 ShadowCameraViewMatrix;
	mat4 lsMatrices[3];
	flat int cascadesCount;
	flat int bVisualizeMode;
	flat float shadowCascadeFarPlanes[3];
} ps_in;

layout(binding = 0) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2DArray depthTexture;

vec2 poissonDisk[4] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760)
	);


float CalculateShadow()
{
	vec4 worldPos = vec4(ps_in.worldPos, 1.0);
	vec4 fragPosViewSpace = ps_in.ShadowCameraViewMatrix * worldPos;
	float ViewSpaceDepthValue = -fragPosViewSpace.z;

	int cascadeCount = ps_in.cascadesCount;
	float cascadePlaneDistances[3] = ps_in.shadowCascadeFarPlanes;

	int layer = -1;
	for (int i = 0; i < cascadeCount; ++i)
	{
		if (ViewSpaceDepthValue < cascadePlaneDistances[i])
		{
			layer = i;
			break;
		}
	}

	if (layer == -1)
	{
		layer = cascadeCount - 1;
	}

	vec4 lsPos = ps_in.lsMatrices[layer] * worldPos;

	vec3 lsPosFinal = lsPos.xyz / lsPos.w;
	// remap from -1..1 to 0..1
	vec3 projCoords = lsPosFinal * 0.5 + 0.5;

	//float shadowSamplerTest = texture(depthTexture, vec3(projCoords.xy, projCoords.z + 0.001));

	// No need to remap this to 0..1 as 0..1 matrices are being used
	float pixelLightSpaceDepth = lsPosFinal.z;

	// 2 different ways of calculating bias 
	float cosTheta = clamp(dot(ps_in.Normal, ps_in.DirectionalLightDirection), 0.0, 1.0);
	float bias = 0.005 * tan(acos(cosTheta));
	bias = clamp(bias, 0.0, 0.05);
	//float bias = max(0.005 * (1.0 - clamp(dot(ps_in.Normal, ps_in.DirectionalLightDirection), 0.0, 1.0)), 0.005);

	float farPlane = 200.0;
	// 	if (layer == cascadeCount)
	// 	{
	// 		bias *= 1 / (farPlane * 0.5f);
	// 	}
	// 	else
	{
		bias *= 1 / (cascadePlaneDistances[layer] * 0.05f);
	}

	vec3 invTexelSize = 1.0 / textureSize(depthTexture, 0); // all layers have the same size, third parameter does not determine layer, it determines mip

// 	float shadow = 0.0;
	//float textureDepth = texture(depthTexture, vec3(projCoords.xy, layer)).r;
// 	if ((pixelLightSpaceDepth - bias > textureDepth))
// 	{
// 		shadow = 1.0;
// 	}

	//float shadow = 0.0;
	//for (int x = -1; x <= 1; ++x)
	//{
	//	for (int y = -1; y <= 1; ++y)
	//	{
	//		float textureDepth = texture(depthTexture, projCoords.xy + vec2(x, y) * invTexelSize).r;
	//		shadow += pixelLightSpaceDepth - bias1 > textureDepth ? 1.0 : 0.0;
	//	}
	//}
	//shadow /= 9.0;

	//float shadow = 0.0;
	//float textureDepth = texture(depthTexture, projCoords.xy).r;
	//if (pixelLightSpaceDepth < 1.0)
	//{
	//	if ((pixelLightSpaceDepth - bias1 > textureDepth))
	//	{
	//		shadow = 1.0;
	//	}
	//}

	//float shadow = 0.0;
	//if (pixelLightSpaceDepth < 1.0)
	//{
	//	for (int i = 0; i < 4; i++) 
	//	{
	//		float textureDepth = texture(depthTexture, projCoords.xy + poissonDisk[i] / 500.0).r;

	//		if (pixelLightSpaceDepth - bias1 > textureDepth) {
	//			shadow += 0.2;
	//		}
	//	}
	//}
	//


	// Vogel disk PCF
	float shadow = 0.0;
	//if (pixelLightSpaceDepth < 1.0)
	{
		const int numSamples = 32;
		float diskRadius = 2.0;

		for (int i = 0; i < numSamples; i++)
		{
			float GoldenAngle = 2.39996322;
			float angle = GoldenAngle * float(i);

			float r = sqrt(float(i) + 0.5) / sqrt(float(numSamples));

			vec2 u = r * vec2(sin(angle), cos(angle));
			vec2 pos = u * diskRadius * invTexelSize.xy;

			float textureDepth = texture(depthTexture, vec3(projCoords.xy + pos, layer)).r;

			if (pixelLightSpaceDepth - bias > textureDepth)
			{
				shadow += 1;
			}
		}

		shadow /= numSamples;
	}

	return shadow;
}

void main()
{
	//float depthValue = texture(quadTexture, inTexCoords).r;
	//FragColor = vec4(vec3(depthValue), 1.0);
	float shadow = CalculateShadow();

	vec4 worldPos = vec4(ps_in.worldPos, 1.0);
	vec4 fragPosViewSpace = ps_in.ShadowCameraViewMatrix * worldPos;
	vec3 color;
 	if (bool(ps_in.bVisualizeMode)) 
 	{
		// Recalculated for debug
		int cascadeCount = ps_in.cascadesCount;
		float cascadePlaneDistances[3] = ps_in.shadowCascadeFarPlanes;
		float ViewSpaceDepthValue = -fragPosViewSpace.z;

		int layer = -1;
		if (ViewSpaceDepthValue > 0.0)
		{
			for (int i = 0; i < cascadeCount; ++i)
			{
				if (ViewSpaceDepthValue < cascadePlaneDistances[i])
				{
					layer = i;
					break;
				}
			}
		}

		if (layer != -1)
		{
 			vec3 cascadeColors[3] = { 
				vec3(1.0, 0.0, 0.0),	// red
				vec3(0.0, 1.0, 0.0),	// green
				vec3(0.0, 0.0, 1.0) };	// blue

 			color = cascadeColors[layer];

		}
		else
		{
			color = vec3(0.0, 0.0, 0.0); // Outside of any available cascade
		}
 	}
 	else
 	{
 		color = (0.8 * ((1 - shadow) * texture(diffuse, ps_in.TexCoords).xyz)) + (0.2 * texture(diffuse, ps_in.TexCoords).xyz);

		//color = vec3(shadow, shadow,shadow);
 		//color = vec3(invTexelSize.xy, 1.0);
 		//color = vec3(textureDepth, textureDepth, textureDepth);
 		//color = vec3(projCoords.xy, layer);
 	}

	color = ps_in.Normal / 2.0 + 0.5;
	FragColor = vec4(color, 1.0);
}