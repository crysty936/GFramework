#include "Transform.h"

Transform::Transform()
	: 
	Translation{ glm::vec3(0.f) },
	Scale{1.f},
	Rotation{1.f, 0.f, 0.f, 0.f }
{

}

Transform::Transform(const Transform& inOther)
	:
	Translation{ inOther.Translation },
	Scale{ inOther.Scale },
	Rotation{ inOther.Rotation }
{
}

Transform::Transform(const glm::vec3 inLocation, const glm::vec3 inRotation, const glm::vec3 inScale)
	:
	Translation{ inLocation },
	Scale{ inScale },
	Rotation{ inRotation }
{}

Transform::Transform(const glm::mat4 inModel)
{
// 	if (translation)
// 	{
// 		// Extract the translation.
// 		translation->x = m[12];
// 		translation->y = m[13];
// 		translation->z = m[14];
// 	}
// 
// 	// Nothing left to do.
// 	if (scale == NULL && rotation == NULL)
// 		return true;
// 
// 	// Extract the scale.
// 	// This is simply the length of each axis (row/column) in the matrix.
// 	Vector3 xaxis(m[0], m[1], m[2]);
// 	float scaleX = xaxis.length();
// 
// 	Vector3 yaxis(m[4], m[5], m[6]);
// 	float scaleY = yaxis.length();
// 
// 	Vector3 zaxis(m[8], m[9], m[10]);
// 	float scaleZ = zaxis.length();
// 
// 	// Determine if we have a negative scale (true if determinant is less than zero).
// 	// In this case, we simply negate a single axis of the scale.
// 	float det = determinant();
// 	if (det < 0)
// 		scaleZ = -scaleZ;
// 
// 	if (scale)
// 	{
// 		scale->x = scaleX;
// 		scale->y = scaleY;
// 		scale->z = scaleZ;
// 	}
// 
// 	// Nothing left to do.
// 	if (rotation == NULL)
// 		return true;
// 
// 	// Scale too close to zero, can't decompose rotation.
// 	if (scaleX < MATH_TOLERANCE || scaleY < MATH_TOLERANCE || fabs(scaleZ) < MATH_TOLERANCE)
// 		return false;
// 
// 	float rn;
// 
// 	// Factor the scale out of the matrix axes.
// 	rn = 1.0f / scaleX;
// 	xaxis.x *= rn;
// 	xaxis.y *= rn;
// 	xaxis.z *= rn;
// 
// 	rn = 1.0f / scaleY;
// 	yaxis.x *= rn;
// 	yaxis.y *= rn;
// 	yaxis.z *= rn;
// 
// 	rn = 1.0f / scaleZ;
// 	zaxis.x *= rn;
// 	zaxis.y *= rn;
// 	zaxis.z *= rn;
// 
// 	// Now calculate the rotation from the resulting matrix (axes).
// 	float trace = xaxis.x + yaxis.y + zaxis.z + 1.0f;
// 
// 	if (trace > 1.0f)
// 	{
// 		float s = 0.5f / sqrt(trace);
// 		rotation->w = 0.25f / s;
// 		rotation->x = (yaxis.z - zaxis.y) * s;
// 		rotation->y = (zaxis.x - xaxis.z) * s;
// 		rotation->z = (xaxis.y - yaxis.x) * s;
// 	}
// 	else
// 	{
// 		// Note: since xaxis, yaxis, and zaxis are normalized, 
// 		// we will never divide by zero in the code below.
// 		if (xaxis.x > yaxis.y && xaxis.x > zaxis.z)
// 		{
// 			float s = 0.5f / sqrt(1.0f + xaxis.x - yaxis.y - zaxis.z);
// 			rotation->w = (yaxis.z - zaxis.y) * s;
// 			rotation->x = 0.25f / s;
// 			rotation->y = (yaxis.x + xaxis.y) * s;
// 			rotation->z = (zaxis.x + xaxis.z) * s;
// 		}
// 		else if (yaxis.y > zaxis.z)
// 		{
// 			float s = 0.5f / sqrt(1.0f + yaxis.y - xaxis.x - zaxis.z);
// 			rotation->w = (zaxis.x - xaxis.z) * s;
// 			rotation->x = (yaxis.x + xaxis.y) * s;
// 			rotation->y = 0.25f / s;
// 			rotation->z = (zaxis.y + yaxis.z) * s;
// 		}
// 		else
// 		{
// 			float s = 0.5f / sqrt(1.0f + zaxis.z - xaxis.x - yaxis.y);
// 			rotation->w = (xaxis.y - yaxis.x) * s;
// 			rotation->x = (zaxis.x + xaxis.z) * s;
// 			rotation->y = (zaxis.y + yaxis.z) * s;
// 			rotation->z = 0.25f / s;
// 		}
// 	}
}

glm::mat4 Transform::GetMatrix() const
{
	if (bDirty)
	{
		bDirty = false;

		glm::mat4 model(1.f);

		model = glm::translate(model, Translation);
		model = model * glm::mat4_cast(Rotation);
		model = glm::scale(model, Scale);

		MatrixCache = model;

		return model;
	}


	return MatrixCache;
}

glm::mat4 Transform::GetRotationOnlyMatrix() const
{
	return glm::mat4_cast(Rotation);
}

void Transform::Rotate(const float inAmount, const glm::vec3 inAxis)
{
	bDirty = true;

	glm::quat additiveRotation = glm::angleAxis(glm::radians(inAmount), inAxis);
	Rotation = Rotation * additiveRotation;
}

Transform Transform::operator*(const Transform& inOther) const
{
	bDirty = true;

	Transform out;

 	out.Translation = inOther.Rotation * (inOther.Scale * this->Translation) + inOther.Translation;
 	out.Rotation = inOther.Rotation * this->Rotation;
 	out.Scale = this->Scale * inOther.Scale;

	return out;
}

