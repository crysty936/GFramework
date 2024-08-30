#pragma once
#include "Math/Transform.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"

using TransformObjPtr = eastl::shared_ptr<class TransformObject>;
/// <summary>
/// An object which contains Transform information.
/// Base class for all in-game entities
/// </summary>
class TransformObject : public eastl::enable_shared_from_this<TransformObject>
{
public:
	TransformObject(const eastl::string& inName);
	virtual ~TransformObject();

	inline void AddChild(TransformObjPtr inTransfObj) { inTransfObj->SetParent(shared_from_this()); }
	inline glm::vec3 GetLocation() { return Location; }
	inline const eastl::vector<TransformObjPtr>& GetChildren() const { return Children; }
	void SetParent(TransformObjPtr& inParent);
	inline eastl::weak_ptr<TransformObject>& GetParent() { return Parent; }

	const Transform GetRelativeTransform() const;
	const Transform& GetAbsoluteTransform() const;

	// Utility Methods
	void Move(const glm::vec3 inMoveVector);
	void Rotate(const float inAmount, const glm::vec3 inAxis);
	void SetRotationDegrees(const glm::vec3 inNewRotation);
	void SetRotationRadians(const glm::vec3 inNewRotation);
	inline glm::vec3 GetRelRotation() const { return Rotation; }
	void SetRelTransform(const Transform& inNewTransf);
	void SetRelativeLocation(const glm::vec3 inRelLoc);
	void SetScale(const glm::vec3 inScale);
	void LookAt(const glm::vec3 inTarget);

	template<typename T>
	void ForEach_Children_Recursive(T inPredicate)
	{
		for (const TransformObjPtr& child : GetChildren())
		{
			child->ForEach_Children_Recursive(inPredicate);

			inPredicate(child);
		}
	}

protected:
	void CalculateAbsTransform() const;
	void MakeTransfDirty() const;
	inline bool IsTranfsDirty() const { return TransfDirty; }
	inline void CleanTranfsDirty() const { TransfDirty = false; }

protected:
	glm::vec3 Location	= { 0.f, 0.f, 0.f };
	glm::vec3 Rotation	= { 0.f, 0.f, 0.f };
	glm::vec3 Scale		= { 1.f, 1.f, 1.f };
	mutable Transform AbsoluteTranfs;
	mutable bool TransfDirty = true;
	eastl::weak_ptr<TransformObject> Parent;
	eastl::vector<TransformObjPtr> Children;

	eastl::string Name;

	friend class Scene;
};