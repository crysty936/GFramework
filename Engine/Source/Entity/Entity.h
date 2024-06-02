#pragma once
#include "Math/Transform.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/vector.h"
#include "TransformObject.h"

using EntityPtr = eastl::shared_ptr<class Entity>;
using EntityIterator = eastl::vector<eastl::shared_ptr<class Entity>>::iterator;

// TODO: Have a CreateEntity function that initializes it and does everything necessary instead of exposing the raw constructor
/**
 * Main Game Entity with Transform and Init and Tick functions
 */
class Entity : public TransformObject
{
public:
	Entity(const eastl::string& inEntityName);
	virtual ~Entity();

	virtual void Init();
	virtual void Tick(const float inDeltaT);

	int32_t EntityId{ ++Entities };

private:
	static int32_t Entities;
};