layout(triangles, invocations = 3) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140, binding = 1) uniform MatricesBuffer
{
	mat4 worldToLightClipMatrices[3];
};

void main()
{
	for (int i = 0; i < 3; ++i)
	{
		gl_Position =
			worldToLightClipMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Position.z = gl_Position.z;

		if (gl_Position.z < 0.0)
		{
			gl_Position.z = 0.001;
		}

		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}