#pragma once
#include "EASTL/shared_ptr.h"
#include "Renderer/Material/RenderMaterial.h"
#include "Entity/TransformObject.h"
#include "Renderer/Drawable/Drawable.h"

// TODO: Remake this and get rid of the horrible IDrawableContainer thing

// Each mesh node has a number of meshes. Those meshes don't have their own location as it's the
// Node that holds the transform for all of them. One Mesh Node to many Render Commands, each Render Command represent a mesh

struct MeshNode : public DrawableObject
{
	MeshNode(const eastl::string& inName);
	virtual ~MeshNode() = default;
	//eastl::vector<Mesh3D> Meshes; // Not used as the meshes are stored as Transform Children
};

class Model3D : public TransformObject , public IDrawableContainer
{
public:
	Model3D(const eastl::string& inModelName);
	virtual ~Model3D();
};