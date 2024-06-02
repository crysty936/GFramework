#include "Math/MortonCode.h"
#include "glm/detail/func_exponential.inl"

static void MortonCodeTesting()
{
	// Morton Code Testing


	eastl::vector<uint16_t> mortonCodes;

	uint32_t testtest = MortonCode2(0xffffffff);

	// Max nr that can fit into 
	constexpr uint32_t bitsPerCoordinate = 8;
	const uint8_t maxNr = (glm::pow(2, bitsPerCoordinate) - 1);

	for (uint8_t x = 0; x < maxNr; ++x)
	{
		for (uint32_t y = 0; y < maxNr; ++y)
		{
			//const uint16_t mortonCode = mortonEncode2_for(x, y);
			const uint16_t mortonCode = MortonCode2(x) | MortonCode2(y) << 1;

			mortonCodes.push_back(mortonCode);

			uint8_t resX = 0;
			uint8_t resY = 0;
			reverseMortonCode2(mortonCode, resX, resY);

			ASSERT(x == resX && y == resY);

		}
	}


	struct Position
	{
		int32_t X = 0;
		int32_t Y = 0;
	};

	eastl::vector<Position> reverseMortonCodes;

	constexpr uint32_t bitsPerCoordinateReverse = 16;
	const uint16_t maxNrReverse = glm::pow(2, bitsPerCoordinateReverse) - 1/* Account for 0*/;
	for (uint16_t i = 0; i < maxNrReverse; ++i)
	{
		uint8_t resX = 0;
		uint8_t resY = 0;
		reverseMortonCode2(i, resX, resY);

		reverseMortonCodes.push_back({ resX, resY });

	}
}