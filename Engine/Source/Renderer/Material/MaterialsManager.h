#pragma once
#include "EASTL/unordered_map.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/string.h"
#include "Core/EngineUtils.h"

class MaterialsManager
{
private:
	MaterialsManager();
	~MaterialsManager();

public:
	static void Init();
	static void Terminate();
	static MaterialsManager& Get();

public:
	/** In the absence of a material nodes based logic editor, material logic will be code written but parameters can be gathered from deserialized data*/
	template<class T = RenderMaterial>
	eastl::shared_ptr<T> GetOrAddMaterial(const eastl::string& inMaterialID, OUT bool& outAlreadyExists);

private:
	eastl::shared_ptr<class RenderMaterial> FindMaterial(const eastl::string& inMaterialID);
	void AddMaterial(const eastl::string& inMatId, eastl::shared_ptr<class RenderMaterial> inNewMat);

private:
	eastl::unordered_map<eastl::string, eastl::shared_ptr<class RenderMaterial>> LoadedMaterials{};
	static MaterialsManager* Instance;
};

template<class T>
eastl::shared_ptr<T> MaterialsManager::GetOrAddMaterial(const eastl::string& inMaterialID, OUT bool& outAlreadyExists)
{
	eastl::shared_ptr<RenderMaterial> mat = FindMaterial(inMaterialID);
	outAlreadyExists = !!mat;

	if (!outAlreadyExists)
	{
		eastl::shared_ptr<T> newMat = eastl::make_shared<T>();
		newMat->Init();
		AddMaterial(inMaterialID, newMat);

		return newMat;
	}

	return eastl::static_shared_pointer_cast<T>(mat);
}
