#pragma once
#include <stdint.h>
#include "Renderer/RHI/RHITypes.h"
#include "VertexInputLayout.h"

enum class RHIBufferType
{
	Unknown,
	Vertex,
	Index,
	Uniform,
};

// Base
class RHIBufferBase
{
public:
	RHIBufferBase(RHIBufferType inBufferType);
	virtual ~RHIBufferBase() = default;

	const RHIBufferType BufferType;
};

// Index
class RHIIndexBuffer : public RHIBufferBase
{
public:
	RHIIndexBuffer();
	RHIIndexBuffer(int32_t inIndicesCount);

public:
	int32_t IndexCount;
};

// Vertex
class RHIVertexBuffer : public RHIBufferBase
{
public:
	RHIVertexBuffer();
	RHIVertexBuffer(eastl::shared_ptr<class RHIIndexBuffer>& inIndexBuffer, const class VertexInputLayout& inLayout);
	inline const VertexInputLayout& GetLayout() const { return Layout; }
	inline int32_t GetIndicesCount() { return IndexBuffer->IndexCount; }

public:
	eastl::shared_ptr<RHIIndexBuffer> IndexBuffer;
	VertexInputLayout Layout;
	size_t AllocatedSize = 0;
};

// Uniform/Constants
class RHIUniformBuffer : public RHIBufferBase
{
public:
	RHIUniformBuffer(size_t inInitSize);

public:
	const size_t InitSize = 0;
	EShaderType BindingType = EShaderType::Sh_Vertex;
};

// One dimensional Array/Texture
class RHITextureBuffer : public RHIBufferBase
{
public:
	RHITextureBuffer(size_t inInitSize);

public:
	const size_t InitSize = 0;
	EShaderType BindingType = EShaderType::Sh_Vertex;
};

