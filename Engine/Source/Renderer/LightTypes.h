#pragma once


struct DirLightData
{


};

struct PointLightData
{
	float Linear;
	float Quadratic;
};

enum class ELightType
{
	Directional,
	Point
};

struct LightData
{
	ELightType Type;

	glm::vec3 Color = glm::vec3(1.f, 1.f, 1.f);

	union {
		DirLightData DirData;
		PointLightData PointData;
	} TypeData;
};
