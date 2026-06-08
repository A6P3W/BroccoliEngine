#pragma once

#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "Utils/Log.h"
#include "Utils/UMath.h"
class AActor;
class AGameModeBase;

class World
{
public:
	World();
	~World();
	void Update(float DeltaTime);
	void Draw();

	ObjectManager* GetObjectManager() { return m_ObjectManager.get(); }
	CollisionSystem* GetCollisionSystem() { return m_CollisionSystem.get(); }
	TimerManager* GetTimerManager() { return m_TimerManager.get(); }
	AGameModeBase* GetGameMode() const { return m_GameMode; }
	template<class T>
	T* SpawnGameMode() {
		T* mode = SpawnActor<T>({0,0},0,true);
		SetGameMode(mode);
		mode->Spawned();
		return mode;
	}

	template<class T>
	T* SpawnActor(const FVector2D& Loc = { 0,0 }, FRotator Rot = 0, bool DeferBeginPlay = false) {
		return m_ObjectManager->SpawnObject<T>(Loc, Rot, DeferBeginPlay);
	}

	void SetSimulating(bool bRunning) { bSimulating = bRunning; }
	bool IsSimulating() const { return bSimulating; }

	bool IsTrendingDown() const { return bTrendingDown; }
private:
	void SetGameMode(AGameModeBase* mode);
	std::unique_ptr<CollisionSystem> m_CollisionSystem = nullptr;
	std::unique_ptr<TimerManager> m_TimerManager = nullptr;
	std::unique_ptr<ObjectManager> m_ObjectManager = nullptr;
	AGameModeBase* m_GameMode = nullptr;

	bool bSimulating = true;

	bool bTrendingDown = false;
};

