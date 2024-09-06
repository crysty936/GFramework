#pragma once
#include "EASTL/shared_ptr.h"
#include "Entity/TransformObject.h"
#include "Renderer/Drawable/Drawable.h"
#include "Renderer/RHI/D3D12/D3D12Resources.h"

struct MeshMaterial
{
	eastl::shared_ptr<D3D12Texture2D> AlbedoMap;
	eastl::shared_ptr<D3D12Texture2D> NormalMap;
	eastl::shared_ptr<D3D12Texture2D> MRMap;
};

// MeshNodes are stored as TransformObject children to the main Model3D

struct MeshNode : public DrawableObject
{
	MeshNode(const eastl::string& inName);
	virtual ~MeshNode() = default;

	eastl::shared_ptr<D3D12VertexBuffer> VertexBuffer;
	eastl::shared_ptr<D3D12IndexBuffer> IndexBuffer;
	//eastl::vector<eastl::shared_ptr<D3D12Texture2D>> Textures;
	uint32_t MatIndex = uint32_t(-1);
};

class Model3D : public TransformObject
{
public:
	Model3D(const eastl::string& inModelName);
	virtual ~Model3D();


	virtual void Init(struct ID3D12GraphicsCommandList* inCommandList) = 0;

	eastl::vector<MeshMaterial> Materials;
};