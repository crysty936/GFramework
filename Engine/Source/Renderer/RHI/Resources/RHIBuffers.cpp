#include "RHIBuffers.h"

RHIBufferBase::RHIBufferBase(RHIBufferType inBufferType)
	: BufferType{inBufferType}
{

}

RHIIndexBuffer::RHIIndexBuffer(int32_t inIndicesCount)
	: RHIBufferBase(RHIBufferType::Index), IndexCount(inIndicesCount)
{

}

RHIIndexBuffer::RHIIndexBuffer()
	: RHIBufferBase(RHIBufferType::Index)
{

}

RHIVertexBuffer::RHIVertexBuffer(eastl::shared_ptr<RHIIndexBuffer>& inIndexBuffer, const VertexInputLayout& inLayout)
	: RHIBufferBase(RHIBufferType::Vertex), IndexBuffer(inIndexBuffer), Layout(inLayout)
{

}

RHIVertexBuffer::RHIVertexBuffer()
	: RHIBufferBase(RHIBufferType::Vertex)
{}

RHIUniformBuffer::RHIUniformBuffer(size_t inInitSize)
	: RHIBufferBase(RHIBufferType::Uniform), InitSize(inInitSize)
{}

RHITextureBuffer::RHITextureBuffer(size_t inInitSize)
	: RHIBufferBase(RHIBufferType::Uniform), InitSize(inInitSize)
{}

