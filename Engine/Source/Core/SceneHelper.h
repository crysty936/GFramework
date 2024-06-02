#pragma once
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Renderer/Drawable/Drawable.h"

class SceneHelper
{
public:
	// Plain C++ Object without any scene presence
	template<typename T, typename... inParamTypes>
	static eastl::shared_ptr<T> CreateObject(inParamTypes... inParams)
	{
		eastl::shared_ptr<T> newObject = eastl::make_shared<T>(std::forward<inParamTypes>(inParams)...);

		return newObject;
	}

	// C++ object that has scene presence(position, parent hierarchy, etc)
	template<typename T, typename... inParamTypes>
	static eastl::shared_ptr<T> CreateEntity(inParamTypes... inParams)
	{
		eastl::shared_ptr<T> newObject = eastl::make_shared<T>(std::forward<inParamTypes>(inParams)...);

		SceneManager& sManager = SceneManager::Get();
		Scene& currentScene = sManager.GetCurrentScene();
		currentScene.AddObject(newObject);

		return newObject;
	}


	// Entity that has a visual representation
  	template<typename T, typename... inParamTypes>
	static eastl::shared_ptr<T> CreateVisualEntity(inParamTypes... inParams)
  	{
        static_assert(std::is_base_of<IDrawableContainer, T>::value, "Type to be created is not a Drawable.");

  		eastl::shared_ptr<T> newObject = eastl::make_shared<T>(std::forward<inParamTypes>(inParams)...);
  
  		SceneManager& sManager = SceneManager::Get();
  		Scene& currentScene = sManager.GetCurrentScene();
  		currentScene.AddObject(newObject);
  
  		using func = void(IDrawableContainer::*)();
  		constexpr func theFunc = &IDrawableContainer::CreateProxy;
  
		T* obj = newObject.get();
		(obj->*theFunc)();
  
  		return newObject;
  	}

// 	template<typename T, typename = std::enable_if_t<std::is_base_of<IDrawableContainer, T>>, typename... inParamTypes>
// 	static eastl::shared_ptr<T> CreateVisualEntity(inParamTypes... inParams)
// 	{
// 		eastl::shared_ptr<T> newObject = eastl::make_shared<T>(std::forward<inParamTypes>(inParams)...);
// 
// 		SceneManager& sManager = SceneManager::Get();
// 		Scene& currentScene = sManager.GetCurrentScene();
// 		currentScene.AddObject(newObject);
// 
// 		// Compile time type deduction and caching member function pointer VTable index to use if type is IDrawable
// 		constexpr bool isDrawable = std::is_base_of<IDrawableContainer, T>();
// 
// 		using func = void(IDrawableContainer::*)();
// 		constexpr func theFunc = &IDrawableContainer::CreateProxy;
// 
//  		if (isDrawable)
//  		{
//  			T* obj = newObject.get();
//  			(obj->*theFunc)();
//  		}
// 
// 		return newObject;
// 	}
};

