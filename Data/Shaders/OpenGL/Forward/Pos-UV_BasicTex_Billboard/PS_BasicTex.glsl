#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
} ps_in;

uniform sampler2D quadTexture;

void main()
{
	vec4 color = texture(quadTexture, ps_in.TexCoords);
	FragColor = color;
}