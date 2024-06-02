#include "Math/BVH.h"
#include <float.h>
#include "Renderer/DrawDebugHelpers.h"

BVHNode::BVHNode() = default;

BVHNode::~BVHNode()
{
	delete LeftNode, RightNode;
}

void BVHNode::DebugDraw() const
{
	BoundingBox.DebugDraw();

	if (LeftNode)
	{
		LeftNode->DebugDraw();
	}

	if (RightNode)
	{
		RightNode->DebugDraw();
	}
}

BVH::BVH() = default;

BVH::~BVH()
{
	delete Root;
}

#define TERMINATION_SIZE 2

void RecursivelyBuildBVH(BVHNode& inNode, const eastl::vector<PathTraceTriangle>& inTriangles, bool& continueRecursion)
{
	if (inTriangles.size() <= TERMINATION_SIZE)
	{
		inNode.Triangles = inTriangles;
		for (const PathTraceTriangle& triangle : inTriangles)
		{
			inNode.BoundingBox += triangle.GetBoundingBox();
		}

		return;
	}

	eastl::vector<glm::vec3> triangleCenters;
	triangleCenters.reserve(inTriangles.size());

	const float invTriangleCount = 1.f / inTriangles.size();

	eastl::vector<PathTraceTriangle> leftSideTriangles;
	eastl::vector<PathTraceTriangle> rightSideTriangles;

	bool validSplit = false;
	int32_t tries = 1;
	while (!validSplit && tries <= 2)
	{
		leftSideTriangles.clear();
		rightSideTriangles.clear();
		glm::vec3 combinedCenter = glm::vec3(0.f, 0.f, 0.f);
		AABB comparisonAABB;
		inNode.BoundingBox = AABB();

		for (const PathTraceTriangle& triangle : inTriangles)
		{
			const glm::vec3 triangleCenter = (triangle.V[0] + triangle.V[1] + triangle.V[2]) * 0.3333333333333333333333f;

			triangleCenters.push_back(triangleCenter);

			const AABB triangleAABB = triangle.GetBoundingBox();
			inNode.BoundingBox += triangleAABB;
			if (tries == 1)
			{
				comparisonAABB += triangleAABB;
			}
			else
			{
				comparisonAABB += triangleCenter;
			}

			combinedCenter += triangleCenter * invTriangleCount;
		}
	
		const float size_x = comparisonAABB.Max.x - comparisonAABB.Min.x;
		const float size_y = comparisonAABB.Max.y - comparisonAABB.Min.y;
		const float size_z = comparisonAABB.Max.z - comparisonAABB.Min.z;

		auto getX = [](const glm::vec3& inVec) -> float {return inVec.x; };
		auto getY = [](const glm::vec3& inVec) -> float {return inVec.y; };
		auto getZ = [](const glm::vec3& inVec) -> float {return inVec.z; };

		using getSizeType = float(*)(const glm::vec3& inVec);

		getSizeType getAxisFunc;

		if (size_y > size_x) {
			if (size_y > size_z) {
				// Longest Axis Y
				//LOG_INFO("Splitting using Y Axis.");
				getAxisFunc = getY;
			}
			else {
				// Longest axis Z
				//LOG_INFO("Splitting using Z Axis.");
				getAxisFunc = getZ;
			}
		}
		else if (size_z > size_x) {
			// Longest axis Z
			//LOG_INFO("Splitting using Z Axis.");
			getAxisFunc = getZ;
		}
		else
		{
			//LOG_INFO("Splitting using X Axis.");
			getAxisFunc = getX;
		}

		// TODO: Nr of triangles at the end will be equal to inTriangles.size()
		// Find a way to optimize this so one array is needed

		leftSideTriangles.reserve(inTriangles.size());
		rightSideTriangles.reserve(inTriangles.size());

		static bool drawSplitCentersDebug = false;

		for (int32_t i = 0; i < inTriangles.size(); ++i)
		{
			const PathTraceTriangle& currentTriangle = inTriangles[i];
			const glm::vec3& triangleCenter = triangleCenters[i];

			const float& comparisonValueTriangle = getAxisFunc(triangleCenter);
			const float& comparisonValueCombinedCenter = getAxisFunc(combinedCenter);
			if (comparisonValueTriangle < comparisonValueCombinedCenter)
			{
				leftSideTriangles.push_back(currentTriangle);

				if (drawSplitCentersDebug)
				{
					DrawDebugHelpers::DrawDebugPoint(triangleCenter, 0.05f, glm::vec3(1.f, 0.f, 0.f), true);
				}
			}
			else
			{
				rightSideTriangles.push_back(currentTriangle);
				if (drawSplitCentersDebug)
				{
					DrawDebugHelpers::DrawDebugPoint(triangleCenter, 0.05f, glm::vec3(0.f, 0.f, 1.f), true);
				}
			}
		}

		validSplit = leftSideTriangles.size() != inTriangles.size() && rightSideTriangles.size() != inTriangles.size();

		//if (!validSplit)
		//{
		//	if (tries == 1)
		//	{
		//		++tries;
		//		LOG_WARNING("Partition by axis failed, attempting second method using centers instead of triangles AABB");
		//	}
		//	else if (tries == 2)
		//	{
		//		ASSERT_MSG(false, "Partition by axis failed, this would lead to infinite recursion, exiting.");

		//		for (const glm::vec3& triangleCenter : triangleCenters)
		//		{
		//			DrawDebugHelpers::DrawDebugPoint(triangleCenter, 0.05f, glm::vec3(0.f, 1.f, 0.f), true);
		//		}

		//		DrawDebugHelpers::DrawDebugPoint(combinedCenter, 0.1f, glm::vec3(1.f, 0.f, 0.f), true);

		//		continueRecursion = false;
		//		return;
		//	}
		//}
	}


	if (!continueRecursion)
		return;

	inNode.LeftNode = new BVHNode();
	RecursivelyBuildBVH(*inNode.LeftNode, leftSideTriangles, continueRecursion);

	if (!continueRecursion)
		return;

	inNode.RightNode = new BVHNode();
	RecursivelyBuildBVH(*inNode.RightNode, rightSideTriangles, continueRecursion);
}


void BVH::Build(const eastl::vector<PathTraceTriangle>& inTriangles)
{
	LOG_INFO("Building BVH.");

	Root = new BVHNode();

	bool recurse = true;
	RecursivelyBuildBVH(*Root, inTriangles, recurse);

	LOG_INFO("BVH Building done.");
}


// Slab Method
// https://tavianator.com/2011/ray_box.html
bool RayIntersectsAABB(const PathTracingRay& inRay, const AABB& inAABB)
{
	const float inv_direction_x = 1.0f / inRay.Direction.x;
	const float inv_direction_y = 1.0f / inRay.Direction.y;
	const float inv_direction_z = 1.0f / inRay.Direction.z;

	float tmin = (inAABB.Min.x - inRay.Origin.x) * inv_direction_x;
	float tmax = (inAABB.Max.x - inRay.Origin.x) * inv_direction_x;

	if (tmin > tmax) std::swap(tmin, tmax);

	float tymin = (inAABB.Min.y - inRay.Origin.y) * inv_direction_y;
	float tymax = (inAABB.Max.y - inRay.Origin.y) * inv_direction_y;

	if (tymin > tymax) std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax)) return false;

	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	float tzmin = (inAABB.Min.z - inRay.Origin.z) * inv_direction_z;
	float tzmax = (inAABB.Max.z - inRay.Origin.z) * inv_direction_z;

	if (tzmin > tzmax) std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax)) return false;

	return true;
}


bool BVH::Intersects(const PathTracingRay& inRay) const
{
	return Root->Intersects(inRay);
}

bool BVHNode::Intersects(const PathTracingRay& inRay) const
{
	if (RayIntersectsAABB(inRay, BoundingBox))
	{
		if(LeftNode)
		{
			return LeftNode->Intersects(inRay) || RightNode->Intersects(inRay);
		}
		else
		{
			for (const PathTraceTriangle& triangle : Triangles)
			{
				if (IntersectsTriangle(inRay, triangle))
				{
					return true;
				}
			}

		}

	}

	return false;
}

bool BVHNode::Trace(const PathTracingRay& inRay, PathTracePayload& outPayload) const
{
	if (RayIntersectsAABB(inRay, BoundingBox))
	{
		if (LeftNode)
		{
			PathTracePayload leftPayload;
			const bool leftHit = LeftNode->Trace(inRay, outPayload);

			PathTracePayload rightPayload;
			const bool rightHit = RightNode->Trace(inRay, rightPayload);

			if (leftHit && leftPayload.Distance < outPayload.Distance)
			{
				outPayload = leftPayload;
			}

			if (rightHit && rightPayload.Distance < outPayload.Distance)
			{
				outPayload = rightPayload;
			}

			return leftHit || rightHit;
		}
		else
		{
			bool bHit = false;
			for (const PathTraceTriangle& triangle : Triangles)
			{
				PathTracePayload currPayload;
				if (TraceTriangle(inRay, triangle, currPayload))
				{
					bHit = true;
					if (currPayload.Distance < outPayload.Distance)
					{
						outPayload = currPayload;
					}
				}
			}

			return bHit;
		}
	}

	return false;
}

float BVH::Trace(const PathTracingRay& inRay, PathTracePayload& outPayload) const
{
	return Root->Trace(inRay, outPayload);
}
