#pragma once
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float2.hpp"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};
