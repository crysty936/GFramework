#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 FragColor;

in VS_OUT
{
	vec2 TexCoords;
	mat4 clipToWorldMatrix;
	vec3 worldPos;
	vec3 VertexNormal;
	mat3 TangentToWorld;
	vec3 modelPos;
} ps_in;

layout(std140, binding = 1) uniform SHDebugStuff
{
	vec3 ObjPos;
	float padding1;
	float Theta;
	float Phi;
} Constants;

const float PI = 3.14159265359f;

#define SH_NUM_BANDS 2
#define SH_COEFFICIENT_COUNT (SH_NUM_BANDS * SH_NUM_BANDS)

//layout(binding = 0) uniform samplerBuffer tbo_texture;

void main()
{
	const vec3 relPos = normalize(ps_in.worldPos - Constants.ObjPos);

	const float ro = length(relPos); // = 1 for sphere or circle of unit radius
	// Z is length of Adjacent and Ro is length of Hypotenuse
	// cos is Adjacent / Hypotenuse, so this is basically acos(cos)
	// Because it's unit length, z is basically the cos value so arc cos of it just returns the current angle, which is what Theta is
	//const float thetaPi = acos(relPos.z / ro);
	const float thetaPi = acos(relPos.z); 
	// Again, tan is Opposite / Adjacent and in the right triangle formed by connecting Y and X, Y is Opposite and X is Adjacent so this is basically atan(tan) to get the angle
	//const float phiPi = atan(relPos.y / relPos.x);
	const float phiPi = atan(relPos.y, relPos.x);

	const float theta = thetaPi / PI;
	float phi = phiPi / PI;
	phi += 1; // -1..1 to 0..2

	//const float normalizedY = relPos.y / ro;
	//FragColor = vec4(normalizedY, normalizedY, normalizedY, 1.0);
	//FragColor = vec4(ps_in.worldPos.x, ps_in.worldPos.y, ps_in.worldPos.z, 1.0);
	//FragColor = vec4(ps_in.modelPos.x, ps_in.modelPos.y, ps_in.modelPos.z, 1.0);
	//FragColor = vec4(Constants.ObjPos, 1.0);
	//FragColor = vec4(phi, phi, phi, 1.0);


	const float thetaDelta = theta - Constants.Theta;
	const float phiDelta = phi - Constants.Phi;
	if ((thetaDelta < 0.03 && thetaDelta > 0.001) && (phiDelta < 0.03 && phiDelta > 0.001))
	{
		vec3 color = vec3(1.0, 0.0, 0.0);
		FragColor = vec4(color.xyz, 1.0);
	}
	else
	{
		FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	}



}