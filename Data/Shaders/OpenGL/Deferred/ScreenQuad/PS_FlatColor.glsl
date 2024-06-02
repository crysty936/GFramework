#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedoSpec;

void main()
{
	vec4 color = vec4(1.0, 1.0, 0.0, 1.0);
	gAlbedoSpec = color;

	gNormal = vec4(0.0, 1.0, 0.0, 0.0);
}