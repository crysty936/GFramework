#pragma once

#include "Core/EngineUtils.h"
#include "EASTL/array.h"
#include "EASTL/vector.h"
#include "glm/ext/vector_float3.hpp"

inline uint16_t mortonEncode2_for(uint8_t x, uint8_t y)
{
	uint16_t answer = 0;

	const uint16_t nrCalcs = (sizeof(uint16_t) * CHAR_BIT) / 2;
	constexpr uint16_t mask = 1u;

	// Buils it from the right to the left
	for (uint16_t i = 0; i < nrCalcs; ++i)
	{
		// Isolate bit in each coordinate, shift to right position and |=
		const uint16_t bitX = (x >> i & mask) << (2 * i);
		const uint16_t bitY = (y >> i & mask) << ((2 * i) + 1);

		answer |= bitX | bitY;
	}

	return answer;
}

inline void reverseMortonCode2(uint16_t inCode, uint8_t& outX, uint8_t& outY)
{
	const uint16_t nrCalcs = (sizeof(uint16_t) * CHAR_BIT) / 2;
	constexpr uint16_t mask = 1u;

	for (uint16_t i = 0; i < nrCalcs; ++i)
	{
		outX |= (inCode & (mask << (2 * i))) >> i;
		outY |= (inCode & (mask << (2 * i + 1))) >> (i + 1);
	}

}

/** Spreads bits to every other. */
inline uint32_t MortonCode2(uint32_t x)
{
	x &= 0x0000ffff;
	x = (x | (x << 8)) & 0x00ff00ff;
	x = (x | (x << 4)) & 0x0f0f0f0f;
	x = (x | (x << 2)) & 0x33333333;
	x = (x | (x << 1)) & 0x55555555;

	return x;
}
