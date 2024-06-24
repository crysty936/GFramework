#pragma once
#include "EASTL/shared_ptr.h"
#include "Renderer/Material/RenderMaterial.h"
#include "Entity/TransformObject.h"
#include "Renderer/Drawable/Drawable.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"

// MeshNodes are stored as TransformObject children to the main Model3D

struct MeshNode : public DrawableObject
{
	MeshNode(const eastl::string& inName);
	virtual ~MeshNode() = default;

	eastl::shared_ptr<D3D12VertexBuffer> VertexBuffer;
	eastl::shared_ptr<D3D12IndexBuffer> IndexBuffer;
	eastl::vector<eastl::shared_ptr<D3D12Texture2D>> Textures;
};

class Model3D : public TransformObject
{
public:
	Model3D(const eastl::string& inModelName);
	virtual ~Model3D();

};