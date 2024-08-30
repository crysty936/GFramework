#pragma once
#include "glm/gtc/quaternion.hpp"

/**
 * Transform containing position, orientation and scale data for entities
 * Order of multiplication for parent child nodes is always ChildTransform * ParentTransform
 */
struct Transform
{
	Transform();
	Transform(const Transform& inOther);
	Transform(const glm::vec3 inLocation, const glm::vec3 inRotation, const glm::vec3 inScale);
	Transform(const glm::mat4 inModel);

	glm::vec3 Translation;
	glm::vec3 Scale;
	glm::quat Rotation;

	glm::mat4 GetMatrix() const;
	glm::mat4 GetRotationOnlyMatrix() const;

	mutable glm::mat4 MatrixCache;
	mutable bool bDirty = true;

	void Rotate(const float inAmount, const glm::vec3 inAxis);

	Transform operator*(const Transform& inOther) const;
	Transform& operator=(const Transform& inOther) = default;
	Transform& operator=(Transform&& inOther) = default;
};