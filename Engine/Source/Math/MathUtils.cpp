#include "Math/MathUtils.h"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"

using vec4i = glm::vec<4, uint8_t>;

//https://marcodiiga.github.io/encoding-normalized-floats-to-rgba8-vectors
//https://www.h-schmidt.net/FloatConverter/IEEE754.html
// Works only for 0..1 floats
// For use mostly in shaders
// What this basically does is isolate parts of the mantissa and store them in the different channels of the 8 bit RGBA.
// It works on the principle that if you multiply the exp by 256, you remove parts of the mantissa from the fraction, because the exponent becomes so big
// that those important bits of the mantissa add integer parts to the final result. The mantissa important bits go from left to right, with left adding most to the value.
// Eg. 0.1f = 0 01111011 1001 1001 1001 1001 1001 101
// sign(1 bit) exp(8 bit) mantissa(23 bits).
// By multiplying by 256, we get 25.6. 0 10000011 1001 1001 1001 1001 1001 101
// Mantissa stays the same. But, the first 4 bits of the mantissa, 1001 only add integer values to the final result now, so if we use fract, we basically isolate the last parts of the mantissa, and how much they add in that "* 256 space"
// From the point we use fract onward, it's not really about the bits themselves but about the value
// First multiplication by 256 remove first 4 bits of the mantissa, second removes next 8, third removes next 8, leaving the final 3.
// So we first multiply by 256 * 256 * 256, we have basically "shifted" so that we only have the last part of the mantissa affecting the fractional value. 
// This "leftover" mantissa will add fractional values only between 0..1, thus being convertable to a 0..255 8 bit by multiplying it with 256, and back to get the value again.
// Thus, by getting fract, we basically get "this value of the original float given by this part of the mantissa, normalized to a 0..1". 
// Each *256 basically represents a space, and to isolate one, we have to remove the previous ones from it, expect the last one, which has nothing to remove.
// So we first multiply by 256 * 256 * 256 to get the value given by the last part of the mantissa(101) isolated in this 0..1 space.
// This gets put as it is in the uint. It can be divided by 256 * 256 * 256 to get the "actual" value brought by that part of the mantissa in the standard space, "unshifted"
// Next, we multiply by 256 * 256, thus we isolate the 2 last parts, (1001 1001 101). We now need to remove what the 101 adds, though, so we don't end up with more in the end given that we add all channels.
// To do this, we basically bring the previous value in this value's "space" by dividing it by 256, so we divide x by 256, to bring it in the 256 * 256 "space" and subtract it.
// We do this for all channels. Last channel always brings the most value in, because it contains the most significant bits of the mantissa.
glm::vec4 PackFloatToVec4i(const float value)
{
	const glm::vec4 bitSh = glm::vec4(256.0f * 256.0f * 256.0f, 256.0f * 256.0f, 256.0f, 1.0f);

	// Isolate parts of the mantissa, first the last 3 bits, then the last 3 + previous 8, then previous + previous 8, then previous + last previous 4.
	// This basically allows those parts of the mantissa to affect the value so they can output a fractional between 0 and 1. This allows encoding that mantissa into 256 values(8 bits).
	// Mathematically, it could be said that the earlier negative powers of 2 of the mantissa, from left to right are given positive powers, thus making them have a non fractional value, thus
	// allowing only the remaining negative parts of mantissa to affect the decimal value, roughly from .0 to 1
	const glm::vec4 expShifted = value * bitSh;

	// This basically grabs that 0..1 value of the leftover negative power of 2 parts of the mantissa.
	glm::vec4 fract = glm::fract(expShifted);


	const float bitMsk = 1.0f / 256.0f;
	// We store in all channels of the rgb, but for subtracting, you subtract the previous one to isolate the current one.
	// For x, it is isolated already, no need to subtract anything
	// What this achieves is basically like, we isolate all parts of the mantissa in 0..1 floats, but we need to remove earlier
	// parts or we will end up with more. To do this, we convert each previous 0..1 mantissa to the current "space".
	// Eg. x was 0..1 in 256*256*256 space, but we divide by 256 to find what its value is in the 256*256 space, to be able to remove it
	// from this one, and so on.
	const glm::vec4 toSubtract = glm::vec4(0.f, fract.x * bitMsk, fract.y * bitMsk, fract.z * bitMsk);

	const glm::vec4 isolated = fract - toSubtract;

	//  For testing
	// 	const glm::vec4 convertBackbitSh = glm::vec4(1.0f / (256.0f * 256.0f * 256.0f), 1.0f / (256.0f * 256.0f), 1.0f / 256.0f, 1.0f);
	// 	const float x = isolated.x * convertBackbitSh.x;
	// 	const float y = isolated.y * convertBackbitSh.y;
	// 	const float z = isolated.z * convertBackbitSh.z;
	// 	const float w = isolated.w * convertBackbitSh.w;
	// 	const float convertedBack = x + y + z + w;

	// To convert to uint8
	//const vec4i intRes = (res * 256.f);
	//return intRes;

	return isolated;
}

float UnpackFloatFromVec4i(const glm::vec4 value)
{
	const glm::vec4 bitSh = glm::vec4(1.0f / (256.0f * 256.0f * 256.0f), 1.0f / (256.0f * 256.0f), 1.0f / 256.0f, 1.0f);
	// Convert to float and divide by 256 to get original isolated mantissa value
	//glm::vec4 actualValue = value;
	//actualValue /= 256.f;
	//return(glm::dot(actualValue, bitSh));

	// For testing
	//const float x = value.x * bitSh.x;
	//const float y = value.y * bitSh.y;
	//const float z = value.z * bitSh.z;
	//const float w = value.w * bitSh.w;
	//const float res = x + y + z + w;

	const float res = glm::dot(value, bitSh);

	return(res);
}

glm::mat4 MathUtils::BuildLookAt(const glm::vec3& inEyeDirection, const glm::vec3& inEyePos, const glm::vec3& inUp)
{
	// 	//https://www.3dgep.com/understanding-the-view-matrix/

	glm::vec3 right = glm::normalize(glm::cross(inUp, inEyeDirection));
	//glm::vec3 right = glm::normalize(glm::cross(inEyeDirection, globalUp)); // For Opengl(right-handed)
	glm::vec3 up = glm::normalize(glm::cross(inEyeDirection, right)); // True orthogonal up. Gram-Shhmidt process.
	//glm::vec3 cameraUp = glm::normalize(glm::cross(right, inEyeDirection)); // For Opengl(right-handed)

	// Rotate inversely related to camera
//  	// Rotation Matrix
//  	/**
//  	* Normally this matrix would be like this:
//  	  	float firstMatrixA[16] =
//   		{
//   			right.x,		right.y,		right.z,		0,
//   			cameraUp.x,		cameraUp.y,		cameraUp.z,		0,
//   			inEyeDirection.x,	inEyeDirection.y,	inEyeDirection.z,		0,
//   			0,				0,				0,				1
//   		};
//  		
//  		And we would need to do:
//  		//rotationMatrix = glm::transpose(rotationMatrix);
//  		Because it's transpose is equal to it's inverse because it the matrix
//  		is orthonormalized
//  
//  		However, we can build it directly transposed
//  	 */

#define BUILD_SEPARATELY 0

#if BUILD_SEPARATELY
	float firstMatrixA[16] =
	{
		right.x,		up.x,		inEyeDirection.x,		0,
		right.y,		up.y,		inEyeDirection.y,		0,
		right.z,		up.z,		inEyeDirection.z,		0,
		0,				0,				0,				1
	};


	glm::mat4 rotationMatrix = glm::mat4(1.f);
	memcpy(glm::value_ptr(rotationMatrix), firstMatrixA, sizeof(glm::mat4));

	// Move inversely related to camera
//  
//  	// Translation Matrix
//  	// Position is negated so that camera is at origin
//  	// 
//  	// Translation inverse is equal to T(-v) T(v)^-1 == T(-v)
// 
// 	//Also, inverse of a TranslationRotation matrix (X)^-1 = (T(t) * R)^-1 = R^t * T(-t), so R^-1 * T^-1, open parantheses mean inverse of operations and inverse of each

	float SecondMatrixA[16] =
	{
		1,		0,		0,		0,
		0,		1,		0,		0,
		0,		0,		1,		0,
		-inEyePos.x,		-inEyePos.y,		-inEyePos.z,		1	// For D3D12
		//-inEyePos.x,		-inEyePos.y,		inEyePos.z,		1	// For OpenGl(right-handed)
	};

	glm::mat4 translationMatrix = glm::mat4(1.f);
	memcpy(glm::value_ptr(translationMatrix), SecondMatrixA, sizeof(glm::mat4));

	glm::mat4 lookAt = rotationMatrix * translationMatrix;// In the shader it's pre-multiplied, meaning that it's first translated and then rotated with camera as center

	return lookAt;
#else
	// Build result of multiplication straight away
	float lookAt[16] =
	{
		right.x,								up.x,									inEyeDirection.x,									0,
		right.y,								up.y,									inEyeDirection.y,									0,
		right.z,								up.z,									inEyeDirection.z,									0,
		-glm::dot(right, inEyePos),				-glm::dot(up, inEyePos),				-glm::dot(inEyeDirection, inEyePos),				1
	};

	glm::mat4 lookAtM = glm::mat4(1.f);
	memcpy(glm::value_ptr(lookAtM), lookAt, sizeof(glm::mat4));

	return lookAtM;
#endif

}



