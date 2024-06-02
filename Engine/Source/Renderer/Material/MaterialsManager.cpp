#include "MaterialsManager.h"
#include "RenderMaterial.h"
#include "Core/EngineUtils.h"
#include <shared_mutex>

using Lock = std::shared_mutex;
using WriteLock = std::unique_lock<Lock>;
using ReadLock = std::shared_lock<Lock>;

static Lock MaterialsMutex;

MaterialsManager* MaterialsManager::Instance = nullptr;

MaterialsManager::MaterialsManager() = default;
MaterialsManager::~MaterialsManager() = default;

void MaterialsManager::Init()
{
	ASSERT(!Instance);

	Instance = new MaterialsManager();
}

void MaterialsManager::Terminate()
{
	ASSERT(Instance);

	delete Instance;
}

MaterialsManager& MaterialsManager::Get()
{
	ASSERT(Instance);

	return *Instance;
}

eastl::shared_ptr<RenderMaterial> MaterialsManager::FindMaterial(const eastl::string& inMaterialID)
{
	ReadLock lock(MaterialsMutex);

	using materialsIterator = const eastl::unordered_map<eastl::string, eastl::shared_ptr<RenderMaterial>>::iterator;
	const materialsIterator& iter = LoadedMaterials.find(inMaterialID);
	const bool materialExists = iter != LoadedMaterials.end();

	if (!materialExists)
	{
		return nullptr;
	}

	return (*iter).second;
}

void MaterialsManager::AddMaterial(const eastl::string& inMatId, eastl::shared_ptr<class RenderMaterial> inNewMat)
{
	WriteLock lock(MaterialsMutex);

	LoadedMaterials.emplace(inMatId, inNewMat);
}

