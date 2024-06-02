#pragma once
#include "Renderer/RHI/Resources/RHIBuffers.h"

// Contains data required for drawing.
// For now just vertex with index buffer
class MeshDataContainer
{
public:
	MeshDataContainer();
	~MeshDataContainer();

	eastl::shared_ptr<class D3D12VertexBuffer> VBuffer;
	eastl::vector<eastl::shared_ptr<class D3D12IndexBuffer>> AdditionalBuffers;
};