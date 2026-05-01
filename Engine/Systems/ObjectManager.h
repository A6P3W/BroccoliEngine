#pragma once
#include <string>
#include "Core/GameObject.h"
#include <vector>
#include <memory>
class ObjectManager
{
public:
	static ObjectManager& GetInstance();
	ObjectManager();
	void Update(float DeltaTime);
	void Draw();

	template<class T ,typename ...Args>
	T* SpawnObject(Args&&... args) {
		auto obj = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = obj.get();
		m_GameObjects.push_back(std::move(obj));
		return ptr;
	}
private:
	std::vector< std::unique_ptr<AGameObject>> m_GameObjects;
};

