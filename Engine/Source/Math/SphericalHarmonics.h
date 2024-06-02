#pragma once

#include "Core/EngineUtils.h"
#include "EASTL/array.h"
#include "EASTL/vector.h"
#include "glm/ext/vector_float3.hpp"

// Number of Spherical Harmonic bands, commonly referred to with the letter l
#define SH_NUM_BANDS 2
#define SH_COEFFICIENT_COUNT (SH_NUM_BANDS * SH_NUM_BANDS)

// Amount of samples used for Monte Carlo integration
#define SQRT_SAMPLE_COUNT 50
#define SH_TOTAL_SAMPLE_COUNT (SQRT_SAMPLE_COUNT * SQRT_SAMPLE_COUNT)

struct SHSample {
	// Sample direction, in sperical coordinates as well as cartesian coordinates
	float Theta;
	float Phi;
	glm::vec3 Direction;

	// SH coefficients that make up the sample
	float Coeffs[SH_COEFFICIENT_COUNT];
};

struct SphericalHarmonics
{
	static void InitSamples(SHSample samples[SH_TOTAL_SAMPLE_COUNT]);


};