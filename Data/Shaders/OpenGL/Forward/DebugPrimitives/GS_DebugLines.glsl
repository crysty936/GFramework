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


	EmitVertex();

	gl_Position = gs_in[0].endPosition;
	gs_out.color = gs_in[0].color;


	EmitVertex();

	EndPrimitive();
}