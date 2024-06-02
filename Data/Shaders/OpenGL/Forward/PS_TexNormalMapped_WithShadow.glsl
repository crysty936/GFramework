#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 viewPos;
	vec3 Normal;
	mat3 TangentToWorld;
} ps_in;

layout(std140, binding = 1) uniform ShadowDataBuffer
{
	mat4 lsMatrices[3];
	mat4 ShadowCameraViewMatrix;
	vec4 DirectionalLightDirection;
	int cascadesCount;
	float shadowCascadeFarPlanes[3];
};

layout(std140, binding = 3) uniform DebugDataBuffer
{
	int bShadowVisualizeMode;
	int bNormalVisualizeMode;
	int bUseNormalMapping;
	int bUseShadows;
};

layout(binding = 0) uniform sampler2D DiffuseMap;
layout(binding = 1) uniform sampler2D NormalMap;
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
	vec4 fragPosViewSpace = ShadowDataBuffer.ShadowCameraViewMatrix * worldPos;
	float ViewSpaceDepthValue = -fragPosViewSpace.z;

	int cascadeCount = ShadowDataBuffer.cascadesCount;
	float cascadePlaneDistances[3] = ShadowDataBuffer.shadowCascadeFarPlanes;

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

	vec4 lsPos = ShadowDataBuffer.lsMatrices[layer] * worldPos;

	vec3 lsPosFinal = lsPos.xyz / lsPos.w;
	// remap from -1..1 to 0..1
	vec3 projCoords = lsPosFinal * 0.5 + 0.5;

	//float shadowSamplerTest = texture(depthTexture, vec3(projCoords.xy, projCoords.z + 0.001));

	// No need to remap this to 0..1 as 0..1 matrices are being used
	float pixelLightSpaceDepth = lsPosFinal.z;

	// 2 different ways of calculating bias 
	float cosTheta = clamp(dot(ps_in.Normal, ShadowDataBuffer.DirectionalLightDirection.xyz), 0.0, 1.0);
	float bias = 0.005 * tan(acos(cosTheta));
	bias = clamp(bias, 0.0, 0.05);
	//float bias = max(0.005 * (1.0 - clamp(dot(ps_in.Normal, ShadowDataBuffer.DirectionalLightDirection.xyz), 0.0, 1.0)), 0.005);

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

vec3 CalcDirLight(vec3 lightDir, vec3 normal, vec3 viewDir)
{
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.5);
	// combine results
	vec3 ambient = 0.2 * vec3(texture(DiffuseMap, ps_in.TexCoords));
	vec3 diffuse = diff * vec3(texture(DiffuseMap, ps_in.TexCoords));
	vec3 specular = spec * vec3(0.2, 0.2, 0.2);

	return (ambient + diffuse + specular);
}


// vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
// {
// 	vec3 lightDir = normalize(light.position - fragPos);
// 	// diffuse shading
// 	float diff = max(dot(normal, lightDir), 0.0);
// 	// specular shading
// 	vec3 reflectDir = reflect(-lightDir, normal);
// 	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
// 	// attenuation
// 	float distance = length(light.position - fragPos);
// 	float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * (distance * distance));
// 	// combine results
// 	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
// 	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
// 	vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
// 	ambient *= attenuation;
// 	diffuse *= attenuation;
// 	specular *= attenuation;
// 	return (ambient + diffuse + specular);
// }
// 

void main()
{
	//float depthValue = texture(quadTexture, inTexCoords).r;
	//FragColor = vec4(vec3(depthValue), 1.0);
	float shadow = CalculateShadow();

	vec4 worldPos = vec4(ps_in.worldPos, 1.0);
	vec4 fragPosViewSpace = ShadowDataBuffer.ShadowCameraViewMatrix * worldPos;
	vec3 color;
 	if (bool(ps_in.bShadowVisualizeMode))
 	{
		// Recalculated for debug
		int cascadeCount = ShadowDataBuffer.cascadesCount;
		float cascadePlaneDistances[3] = ShadowDataBuffer.shadowCascadeFarPlanes;
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
	else if (bool(ps_in.bNormalVisualizeMode))
	{
		vec3 mapNormal = texture(NormalMap, ps_in.TexCoords).xyz;
		mapNormal = mapNormal * 2.0 - 1.0;
		vec3 wsNormal = normalize(ps_in.TangentToWorld * mapNormal);
		color = wsNormal;
	}
	else
 	{
		if (!bool(ps_in.bUseShadows))
		{
			shadow = 0.0;
		}

		// modify to 0 = fully shadowed, 1 = fully lit
		float shadowModifier = 1 - shadow;

		vec3 ambientColor = texture(DiffuseMap, ps_in.TexCoords).xyz;
		if (bool(DebugDataBuffer.bUseNormalMapping))
		{
// 			mat3 WorldToTangent = transpose(ps_in.TangentToWorld);
// 
// 			vec2 glMappedTexCoords = vec2(ps_in.TexCoords.x, 1.0 - ps_in.TexCoords.y);
			vec3 mapNormal = texture(NormalMap, ps_in.TexCoords).xyz;
			mapNormal = mapNormal * 2.0 - 1.0;
// 			vec3 tsLightDir = normalize(WorldToTangent * ps_in.DirectionalLightDirection);
// 			float diffPower = max(dot(mapNormal, tsLightDir), 0.3);
// 
// 			vec3 dirLightColor = diffPower * texture(DiffuseMap, ps_in.TexCoords).xyz;

			vec3 wsNormal = normalize(ps_in.TangentToWorld * mapNormal);
			color = shadowModifier * CalcDirLight(ps_in.DirectionalLightDirection, wsNormal, normalize(ps_in.viewPos - ps_in.worldPos));
		}
		else
		{
			vec3 wsNormal = normalize(ps_in.Normal);
			color = shadowModifier * CalcDirLight(ps_in.DirectionalLightDirection, wsNormal, normalize(ps_in.viewPos - ps_in.worldPos));
		}
 	}

	FragColor = vec4(color, 1.0);
}