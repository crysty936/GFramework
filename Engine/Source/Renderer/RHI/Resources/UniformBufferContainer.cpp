#include "UniformBufferContainer.h"
#include "RHIBuffers.h"
#include "Renderer/RHI/RHI.h"

void UniformBufferContainer::AddData(const void* inData, const size_t inSize, const bool inForceAlign)
{
	// Scalars and Booleans have aligment of 4 and pack together
	// Vectors can be 8 or 16
	// Each element of an array has a base aligment of 16

	size_t finalSize = inSize;
 	if (inSize > 8 || inForceAlign)
 	{
 		// If a vec2 or greater/element of an array is being added
 		if (Counter % 16 > 0)
 		{
 			// If the current aligment is off because of packing scalars or similar
 			// Correct the buffer

			const size_t multiplier = Counter / 16;
 			Counter = (multiplier + 1) * 16;
 			UniformsDataCache.resize(Counter);
 		}
 	}

	if (UniformsDataCache.size() < (Counter + finalSize))
	{
		UniformsDataCache.resize(Counter + finalSize);
	}

	// Allow adding of padding
	if (inData)
	{
		memcpy(&UniformsDataCache[Counter], inData, inSize);
	}
	Counter += finalSize;
}

void UniformBufferContainer::UpdateData(const EShaderType inBufferType, const int32_t inBufferNr)
{
	if (!RHIBuffer)
	{
		// Buffer size should be a multiple of 16
		size_t bufferSize = UniformsDataCache.size();
		if ((bufferSize % 16) > 0)
		{
			// Round size to the next 16 multiple
			const size_t multiplier = bufferSize / 16;
			bufferSize = (multiplier + 1) * 16;
		}

		//RHIBuffer = RHI::Get()->CreateUniformBuffer(bufferSize);
		RHIBuffer->BindingType = inBufferType;
	}

	//RHI::Get()->UniformBufferUpdateData(*RHIBuffer, UniformsDataCache.data(), UniformsDataCache.size(), inBufferNr);
}

void UniformBufferContainer::Clear()
{
	// Reset data counter
	Counter = 0;
}

void UniformBufferContainer::Bind()
{
	//RHI::Get()->BindUniformBuffer(*RHIBuffer);
}

void UniformBufferContainer::Unbind()
{
	//RHI::Get()->UnbindUniformBuffer(*RHIBuffer);
}
