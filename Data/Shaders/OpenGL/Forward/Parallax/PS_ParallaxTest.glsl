#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 FragWorldPos;
	vec3 FragTangentPos;
	vec3 Normal;
	mat3 WorldToTangent;
	flat vec3 DirectionalLightDirectionTS;
	flat int bNormalVisualizeMode;
	flat int bUseNormalMapping;
	flat int bUseParallaxMapping;
	flat float ParallaxHeightScale;
	vec3 TSViewPos;
} ps_in;

layout(binding = 0) uniform sampler2D DiffuseMap;
layout(binding = 1) uniform sampler2D NormalMap;
layout(binding = 2) uniform sampler2D DispMap;

vec2 OcclusionParallaxMapping(vec2 texCoords, vec3 viewDir, out float finalDepth)
{
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numSamples = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	const float layerDepth = 1.0 / numSamples;

	float currentLayerDepth = 0.0;
	vec2 deltaTexCoords = ps_in.ParallaxHeightScale * viewDir.xy / viewDir.z / numSamples;

	vec2 currentTextCoords = texCoords;
	float currentDepthMapValue = texture(DispMap, texCoords).r;

	while (currentDepthMapValue > currentLayerDepth)
	{
		currentLayerDepth += layerDepth;
		currentTextCoords -= deltaTexCoords;
		currentDepthMapValue = texture(DispMap, currentTextCoords).r;
	}

	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTextCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(DispMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTextCoords * (1.0 - weight);
	finalDepth = currentLayerDepth + beforeDepth * weight + afterDepth * (1.0 - weight);

	return finalTexCoords;
}

vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir)
{
 	const float minLayers = 16.0;
 	const float maxLayers = 64.0;
 	float numSamples = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	//const float numSamples = 32;
	const float layerDepth = 1.0 / numSamples;

	float currentLayerDepth = 0.0;
	vec2 deltaTexCoords = ps_in.ParallaxHeightScale * viewDir.xy / viewDir.z / numSamples;

	vec2 currentTextCoords = texCoords;
	float currentDepthMapValue = texture(DispMap, texCoords).r;

	while (currentDepthMapValue > currentLayerDepth)
	{
		currentLayerDepth += layerDepth;
		currentTextCoords -= deltaTexCoords;
		currentDepthMapValue = texture(DispMap, currentTextCoords).r;
	}
 
 	return currentTextCoords;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	const float initialHeight = texture(DispMap, ps_in.TexCoords).r;

	// Multiply p based on z and height, control with ParallaxHeightScale
	const vec2 texCoordOffset = viewDir.xy / viewDir.z * initialHeight * ps_in.ParallaxHeightScale;

	// Everything is in tangent space so x and y of p are as UV's
	return texCoords - texCoordOffset;
}

float ParallaxHardShadowing(vec3 lightDir, vec2 initialTexCoords, float initialDepth)
{
	float shadowMultiplier = 1;

	const float minLayers = 15;
	const float maxLayers = 30;

	// calculate lighting only for surface oriented to the light source
	if (dot(vec3(0, 0, 1), lightDir) > 0)
	{
		// initial params

		// nr of layers depending on angle between face and light dir in tangent space
		float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), lightDir)));
		float perLayerDepth = initialDepth / numLayers;
		vec2 texStep = ps_in.ParallaxHeightScale * lightDir.xy / lightDir.z / numLayers;


		// operation used params
		
		float currentLayerDepth = initialDepth - perLayerDepth;
		vec2 currentTextureCoords = initialTexCoords + texStep;
		float depthFromTexture = texture(DispMap, currentTextureCoords).r;

		while (currentLayerDepth > 0)
		{
			if (currentLayerDepth > depthFromTexture)
			{
				shadowMultiplier = 0;
				break;
			}

			currentLayerDepth -= perLayerDepth;
			currentTextureCoords += texStep;
			depthFromTexture = texture(DispMap, currentTextureCoords).r;
		}
	}

	// 1 for fully lit, 0 for fully shadowed
	return shadowMultiplier;
}

float ParallaxSoftShadowing(vec3 lightDir, vec2 initialTexCoords, float initialDepth)
{
	float shadowMultiplier = 1;

	const float minLayers = 15;
	const float maxLayers = 30;

	// calculate lighting only for surface oriented to the light source
	if (dot(vec3(0, 0, 1), lightDir) > 0)
	{
		shadowMultiplier = 0;

		// initial params

		// nr of layers depending on angle between face and light dir in tangent space
		float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), lightDir)));
		float perLayerDepth = initialDepth / numLayers;
		vec2 texStep = ps_in.ParallaxHeightScale * lightDir.xy / lightDir.z / numLayers;

		// operation used params

		float numSamplesUnderSurface = 0;
		float currentLayerDepth = initialDepth - perLayerDepth;
		vec2 currentTextureCoords = initialTexCoords + texStep;
		float depthFromTexture = texture(DispMap, currentTextureCoords).r;
		int stepIndex = 1;

		while (currentLayerDepth > 0)
		{
			// depth is measured, so if this layer's depth is greater, it means that the point from the texture is higher, thus in shadow
			if (currentLayerDepth > depthFromTexture)
			{
				numSamplesUnderSurface += 1;

				// calculate the multiplier based on the depth difference, based on the importance of the current sample(how far along the layers it is)
				float newShadowMultiplier = (currentLayerDepth - depthFromTexture) *
					(1.0 - stepIndex / numLayers);

				shadowMultiplier = max(shadowMultiplier, newShadowMultiplier);
			}

			stepIndex += 1;
			currentLayerDepth -= perLayerDepth;
			currentTextureCoords += texStep;
			depthFromTexture = texture(DispMap, currentTextureCoords).r;
		}

		if (numSamplesUnderSurface < 1)
		{
			// No depth less then that has been found along the light direction, thus fully lit
			shadowMultiplier = 1;
		}
		else
		{
			shadowMultiplier = 1.0 - (shadowMultiplier * 2.0);
		}
	}


	// 1 for fully lit, 0 for fully shadowed
	return shadowMultiplier;
}

void main()
{
	vec3 color;
	if (bool(ps_in.bNormalVisualizeMode))
	{
		vec3 mapNormal = texture(NormalMap, ps_in.TexCoords).xyz;
		// to direction vector
		mapNormal = mapNormal * 2.0 - 1.0;
		vec3 wsNormal = normalize(transpose(ps_in.WorldToTangent) * mapNormal);
		// back to color
		wsNormal = wsNormal * 0.5 + 0.5;

		color = wsNormal;
	}
	else
 	{
		vec2 texCoords = ps_in.TexCoords;

		float shadowing = 1.0;

		if (bool(ps_in.bUseParallaxMapping))
		{
			float finalDepth = 0.0;
			texCoords = OcclusionParallaxMapping(ps_in.TexCoords, ps_in.FragWorldPos - ps_in.TSViewPos, finalDepth);
  			if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
  				discard;

			shadowing = ParallaxSoftShadowing(ps_in.DirectionalLightDirectionTS, ps_in.TexCoords, finalDepth);
		}

		vec3 diffColor = texture(DiffuseMap, texCoords).xyz;
		if (bool(ps_in.bUseNormalMapping))
		{
			vec3 mapNormal = texture(NormalMap, texCoords).xyz;
			mapNormal = mapNormal * 2.0 - 1.0;

			float diffPower = max(dot(mapNormal, ps_in.DirectionalLightDirectionTS), 0.3);
			vec3 dirLightColor = diffPower * diffColor;
 			color = shadowing * dirLightColor + 0.2 * diffColor;
		}
		else
		{
			color = shadowing * diffColor;
		}
		
 	}

	//vec3 color = texture(DispMap, ps_in.TexCoords).xyz;

	FragColor = vec4(color, 1.0);
}