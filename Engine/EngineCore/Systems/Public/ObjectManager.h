#pragma once
#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include "Utils/UMath.h"
#include "Utils/Log.h"
class AActor;
class ObjectManager
{
public:
	ObjectManager();
	~ObjectManager();

	static void Initialize();
	static void Terminate();


	void Update(float DeltaTime);
	void Draw();

	const std::vector<std::unique_ptr<AActor>>& GetAllActors() const { return m_Actors; }

	template<class T, std::enable_if_t<std::is_base_of_v<AActor, T>, int> = 0>
	T* SpawnObject(const FVector2D& location = FVector2D::ZeroVector, FRotator rotation = 0.0f,bool DeferBeginPlay = false) {
		std::unique_ptr<T> obj;
		
		if constexpr (std::is_constructible_v<T, const FVector2D&, FRotator>) {
			obj = std::make_unique<T>(location, rotation);
		}
		else {
			obj = std::make_unique<T>();
		}
		T* ptr = obj.get();
		m_Actors.push_back(std::move(obj));
		ptr->SetActorLocation(location);
		ptr->SetActorRotation(rotation);

		if (!DeferBeginPlay)ptr->Spawned();
		
		M_LOG(ptr->GetActorClassName());
		return ptr;
	}
	void ClearAllObjects();
private:
	std::vector<std::unique_ptr<AActor>> m_Actors;
};

