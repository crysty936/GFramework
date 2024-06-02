#include "Model3D.h"

MeshNode::MeshNode(const eastl::string& inName)
	: DrawableObject(inName)
{

}

Model3D::Model3D(const eastl::string& inModelName)
	: TransformObject(inModelName)
{}
Model3D::~Model3D() = default;

