layout(points) in;
layout(line_strip, max_vertices = 2) out;

in VS_OUT{
    vec4 endPosition;
    vec3 color;
} gs_in[];

out GS_OUT{
	vec3 color;
} gs_out;


void main()
{
	gl_Position = gl_in[0].gl_Position;
	gs_out.color = gs_in[0].color;

	// Stick far away lines to the far plane
	if (gl_Position.z / gl_Position.w > 1.f)
	{
		gl_Position.z = 0.99f * gl_Position.w;
	}
	EmitVertex();

	gl_Position = gs_in[0].endPosition;
	gs_out.color = gs_in[0].color;

	// Stick far away lines to the far plane
	if (gl_Position.z / gl_Position.w > 1.f)
	{
		gl_Position.z = 0.99f * gl_Position.w;
	}
	EmitVertex();

	EndPrimitive();
}