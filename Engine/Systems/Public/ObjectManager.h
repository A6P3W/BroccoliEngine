#pragma once
#include <string>
#include "Core/Public/GameObject.h"
#include <vector>
#include <memory>
#include <type_traits>
class ObjectManager
{
public:
	static ObjectManager& GetInstance();
	ObjectManager();
	void Update(float DeltaTime);
	void Draw();


  template<class T, std::enable_if_t<std::is_base_of_v<AGameObject, T>, int> = 0>
	T* SpawnObject(const FVector2D& location = FVector2D::ZeroVector, FRotator rotation = 0.0f) {
		std::unique_ptr<T> obj;
		if constexpr (std::is_constructible_v<T, const FVector2D&, FRotator>) {
			obj = std::make_unique<T>(location, rotation);
		} else {
			obj = std::make_unique<T>();
		}
		T* ptr = obj.get();
		m_UpdateAbleObject.push_back(std::move(obj));
     m_RenderAbleObject.push_back(ptr);
		return ptr;
	}
private:
	std::vector< std::unique_ptr<MUpdateableObject>> m_UpdateAbleObject;
	std::vector<AGameObject*> m_RenderAbleObject;
};

