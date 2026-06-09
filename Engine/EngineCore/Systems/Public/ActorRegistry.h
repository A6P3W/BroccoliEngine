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

	// ゲーム側から呼ぶ登録関数
	template<class T>
	void Register()
	{
		std::string className = T::StaticClassName();
		m_factories[className] = [](World* world, const FVector2D& loc, FRotator rot) -> AActor* {
			return world->SpawnActor<T>(loc, rot, true);
		};
		m_classNames.push_back(className);
	}

	// クラス名からスポーン
	AActor* Spawn(
		World* world,
		const std::string& className,
		const FVector2D& loc = FVector2D::ZeroVector,
		FRotator rot = 0.0f
	)
	{
		auto it = m_factories.find(className);
		if (it == m_factories.end()) return nullptr;
		return it->second(world, loc, rot);
	}

	const std::vector<std::string>& GetClassNames() const { return m_classNames; }

	bool Contains(const std::string& className) const
	{
		return m_factories.count(className) > 0;
	}

private:
	ActorRegistry() = default;
	std::unordered_map<std::string, FactoryFn> m_factories;
	std::vector<std::string> m_classNames;
};
