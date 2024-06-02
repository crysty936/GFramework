#include "Math/SphericalHarmonics.h"
#include "Renderer/DrawDebugHelpers.h"
#include "MathUtils.h"
#include "SphericalHarmonicsRotation.h"
#include "glm/gtc/quaternion.inl"

struct SHRotationMatrix
{
public:
	SHRotationMatrix() = default;
	~SHRotationMatrix() = default;

	inline void SetOrder(int inL)
	{
		L = inL;
		RowSize = 2 * inL + 1;


	}

	inline void SetValue(int inRow, int inCol, float inValue)
	{
		inRow += L;
		inCol += L;

		ASSERT(inRow >= 0 && inRow < RowSize);
		ASSERT(inCol >= 0 && inCol < RowSize);

		Data[inRow * RowSize + inCol] = inValue;
	}


	inline float operator()(int inRow, int inCol) const 
	{
		inRow += L;
		inCol += L;

		return Data[inRow * RowSize + inCol];
	}


private:
	int32_t L = 0;
	int32_t RowSize = 0; // Max (2 * l + 1), min 1

	float Data[(2 * SH_NUM_BANDS + 1) * (2 * SH_NUM_BANDS + 1)];
};

inline float KroneckerDelta(const int32_t inA, const int32_t inB)
{
	return inA == inB ? 1.f : 0.f;
}

inline float Calculate_small_u(const int32_t l, const int32_t n, const int32_t m)
{
	if (abs(n) < l)
	{
		return sqrt(
			(float)((l + m) * (l - m)) /
			(float)((l + n) * (l - n))
		);
	}
	else
	{
		return sqrt(
			(float)((l + m) * (l - m)) /
			(float)((2 * l) * (2 * l - 1))
		);
	}
}

inline float Calculate_small_v(const int32_t l, const int32_t m, const int32_t n)
{
	if (abs(n) < l)
	{
		return 0.5f * sqrt(
			(float)((1.0f + KroneckerDelta(m, 0)) * (l + abs(m) - 1) * (l + abs(m))) /
			(float)((l + n) * (l - n))) *
			(1.0f - 2.0f * KroneckerDelta(m, 0));
	}
	else
	{
		return 0.5f * sqrt(
			(float)((1.0f + KroneckerDelta(m, 0)) * (l + abs(m) - 1) * (l + abs(m))) /
			(float)((2 * l) * (2 * l - 1))) *
			(1.0f - 2.0f * KroneckerDelta(m, 0));
	}
}

inline float Calculate_small_w(const int32_t l, const int32_t m, const int32_t n)
{
	if (abs(n) < l)
	{
		return -0.5f * sqrt(
			(float)((l - abs(m) - 1) * (l - abs(m))) /
			(float)((l + n) * (l - n))) *
			(1.0f - KroneckerDelta(m, 0));
	}
	else
	{
		return -0.5f * sqrt(
			(float)((l - abs(m) - 1) * (l - abs(m))) /
			(float)((2 * l) * (2 * l - 1))) *
			(1.0f - KroneckerDelta(m, 0));
	}
}


inline float Calculate_P(const SHRotationMatrix& R /*Base RotationMatrix*/, const SHRotationMatrix& M /*Prev Matrix*/, 
	const int32_t l, const int32_t i, const int32_t a, const int32_t b)
{
	if (abs(b) < l)
	{
		return R(i, 0) * M(a, b);
	}
	else if (b == l)
	{
		return R(i, 1) * M(a, l - 1) - R(i, -1) * M(a, -l + 1);
	}
	else if (b == -l)
	{
		return R(i, 1) * M(a, -l + 1) + R(i, -1) * M(a, l - 1);
	}

	ASSERT(0);

	return 0.f;
}

inline float Calculate_Big_U(const SHRotationMatrix& baseMatrix, const SHRotationMatrix& prevMatrix, const int32_t l, const int32_t m, const int32_t n)
{
	return Calculate_P(baseMatrix, prevMatrix, l, 0, m, n);
}

inline float Calculate_Big_V(const SHRotationMatrix& baseMatrix, const SHRotationMatrix& prevMatrix, const int32_t l, const int32_t m, const int32_t n)
{
	constexpr float sqrt_2 = 1.41421356237f; // Square root of 2

	if (m == 0)
	{
		return Calculate_P(baseMatrix, prevMatrix, l, 1, 1, n) + Calculate_P(baseMatrix, prevMatrix, l, -1, -1, n);
	}
	else if (m > 0)
	{
		// Branch because of the kronecker delta in the original formula
		if (m == 1)
		{
			return Calculate_P(baseMatrix, prevMatrix, l, 1, m - 1, n) * sqrt_2;
		}
		else
		{
			return Calculate_P(baseMatrix, prevMatrix, l, 1, m - 1, n) - Calculate_P(baseMatrix, prevMatrix, l, -1, -m + 1, n);
		}
	}
	else if (m < 0)
	{
		// @NOTE: Both Green's and Ivanic' papers are wrong in this case!
		// Even in Ivanic' Errata this is still wrong

		// Branch because of the kronecker delta in the original formula
		if (m == -1)
		{
			return Calculate_P(baseMatrix, prevMatrix, l, -1, -m - 1, n) * sqrt_2;
		}
		else
		{
			return Calculate_P(baseMatrix, prevMatrix, l, 1, m + 1, n) + Calculate_P(baseMatrix, prevMatrix, l, -1, -m - 1, n);
		}
	}

	ASSERT(0);

	return 0.f;
}

inline float Calculate_Big_W(const SHRotationMatrix& baseMatrix, const SHRotationMatrix& prevMatrix, const int32_t l, const int32_t m, const int32_t n)
{
	ASSERT(m != 0);

	if (m > 0)
	{
		return Calculate_P(baseMatrix, prevMatrix, l, 1, m + 1, n) + Calculate_P(baseMatrix, prevMatrix, l, -1, -m - 1, n);
	}
	else
	{
		// m < 0
		return Calculate_P(baseMatrix, prevMatrix, l, 1, m - 1, n) - Calculate_P(baseMatrix, prevMatrix, l, -1, -m + 1, n);
	}

}

void SphericalHarmonicsRotation::Init()
{




}

void SphericalHarmonicsRotation::Rotate(const glm::quat& inRot, const eastl::vector<glm::vec4>& inCoeffs, eastl::vector<glm::vec4>& outRotatedCoeffs)
{
	const glm::mat3 rotationMat = glm::mat3_cast(inRot);

	// Base rotation matrix that all others are derived from
	SHRotationMatrix R;
	R.SetOrder(1);

	R.SetValue(-1, -1, rotationMat[1][1]); R.SetValue(-1, 0, rotationMat[2][1]); R.SetValue(-1, 1, rotationMat[0][1]);
	R.SetValue(0, -1, rotationMat[1][2]); R.SetValue(0, 0, rotationMat[2][2]); R.SetValue(0, 1, rotationMat[0][2]);
	R.SetValue(1, -1, rotationMat[1][0]); R.SetValue(1, 0, rotationMat[2][0]); R.SetValue(1, 1, rotationMat[0][0]);

	outRotatedCoeffs.resize(inCoeffs.size());

	// First harmonic remains unchaged(rotation matrix for it is 1)
	outRotatedCoeffs[0] = inCoeffs[0];

	SHRotationMatrix matrices[2];

	// Initialize first matrix as a 1x1 matrix containing 1, because first harmonic remains unchanged
	matrices[0].SetOrder(0);
	matrices[0].SetValue(0, 0, 1.0f);


	int32_t matrixIndex = 1;
	int32_t previousMatrix = 0;
	for (int32_t l = 1; l < SH_NUM_BANDS; ++l)
	{
		const int32_t matrixIndex = l & 1; // Equivalent to Modulo 2
		//const int32_t previousMatrix = int32_t(!matrixIndex); // Basically other matrix

		matrices[matrixIndex].SetOrder(l);


		for (int32_t m = -l; m <= l; ++m)
		{
			for (int32_t n = -l; n <= l; ++n)
			{
				const float u = Calculate_small_u(l, m, n);
				const float v = Calculate_small_v(l, m, n);
				const float w = Calculate_small_w(l, m, n);

				float M_mn = 0.f;

				// Only calculate U, V, W if their respective u, v, w are non-zero
				// Optimisation but also, zero u, v or w cause U, V, W to go index of bounds when calculated

				if (u)
				{
					M_mn += u * Calculate_Big_U(R, matrices[previousMatrix], l, m, n);
				}

				if (v)
				{
					M_mn += v * Calculate_Big_V(R, matrices[previousMatrix], l, m, n);
				}

				if (w)
				{
					M_mn += w * Calculate_Big_W(R, matrices[previousMatrix], l, m, n);
				}

				matrices[matrixIndex].SetValue(m, n, M_mn);
			}
		}

		// Rotation coefficients do not interact with other bounds
		// Modify the current band's coefficients using the resulted matrix
		const int32_t offset = l * (l + 1); // Band we are affecting
		for (int32_t i = -l; i < l; ++i)
		{
			glm::vec4 sum(0.f, 0.f, 0.f, 0.f);

			for (int32_t j = -l; j <= l; ++j)
			{
				sum += matrices[matrixIndex](i, j) * inCoeffs[offset + j];
			}

			outRotatedCoeffs[offset + i] = sum;
		}

		previousMatrix = matrixIndex;
	}
}

