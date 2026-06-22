#pragma once
#include "ObjectManager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "UMath.h"
#include "World.h"
class AActor;
class ActorRegistry
{
public:
	using FactoryFn = std::function<AActor* (World*, const FVector2D&, FRotator)>;

	static ActorRegistry& GetInstance()
	{
		static ActorRegistry instance;
		return instance;
	}

	template<class T>
	void Register()
	{
		std::string className = T::StaticClassName();
		Factories[className] = [](World* world, const FVector2D& loc, FRotator rot) -> AActor* {
			return world->SpawnActor<T>(loc, rot, true);
			};
		ClassNames.push_back(className);
	}

	// クラス名からスポーン
	AActor* Spawn(
		World* world,
		const std::string& className,
		const FVector2D& loc = FVector2D::ZeroVector,
		FRotator rot = FRotator(0)
	)
	{
		auto it = Factories.find(className);
		if (it == Factories.end()) return nullptr;
		return it->second(world, loc, rot);
	}

	const std::vector<std::string>& GetClassNames() const { return ClassNames; }

	bool Contains(const std::string& className) const
	{
		return Factories.count(className) > 0;
	}

private:
	ActorRegistry() = default;
	std::unordered_map<std::string, FactoryFn> Factories;
	std::vector<std::string> ClassNames;
};
