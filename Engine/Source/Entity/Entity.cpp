#include "Entity.h"

int32_t Entity::Entities = 0;

Entity::Entity(const eastl::string& inEntityName)
	: TransformObject(inEntityName) {}
Entity::~Entity() = default;

void Entity::Init()
{

}

void Entity::Tick(const float inDeltaT)
{
}
