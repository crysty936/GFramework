
namespace RGBA8_UNORM
{
	uint Pack(float4 value)
	{
		uint r = uint(round(saturate(value.r) * 255.0f));
		uint g = uint(round(saturate(value.g) * 255.0f));
		uint b = uint(round(saturate(value.b) * 255.0f));
		uint a = uint(round(saturate(value.a) * 255.0f));

		uint result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

	float4 Unpack(uint value)
	{
		float4 res;
		// Get the necessaary 8 bits, from right to left, then multiply by 256 scale
		res.r = ((value << 24) >> 24) * 256.0f;
		res.g = ((value << 16) >> 24) * 256.0f;
		res.b = ((value << 8) >> 24)  * 256.0f;
		res.a = ((value << 0) >> 24)  * 256.0f;
	
		return res;
	}
}