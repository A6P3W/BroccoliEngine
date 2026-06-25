#include "World.h"

#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "CameraComponent.h"
#include "ReplicationSystem.h"
#include "SceneManager.h"
#include "World.h"
World::World()
{
    ObjectManager = std::make_unique<MObjectManager>();
    CollisionSystem = std::make_unique<MCollisionSystem>();
    SoundManager = std::make_unique<MSoundManager>();
    TimerManager = std::make_unique<MTimerManager>();
    
	ObjectManager->SetWorld(this);
	ReplicationSystem = std::make_unique<MReplicationSystem>(this);
}

World::~World()
{
	bTrendingDown = true;
}

void World::Update(float DeltaTime)
{

    if (ObjectManager)ObjectManager->Update(DeltaTime);
    if (ReplicationSystem)ReplicationSystem->Update();
    if (ObjectManager)ObjectManager->RemovePendingDestroy();
    if (bSimulating) {
        if (TimerManager)TimerManager->Update(DeltaTime);
        if (CollisionSystem)CollisionSystem->CheckCollisions();
    }
}

void World::Draw()
{
    if (ObjectManager) ObjectManager->Draw();
}

void World::SetGameMode(AGameModeBase* mode)
{
    GameMode = mode;
}

ENetMode World::GetNetMode() const
{
    return NetMode;
}

void World::SetNetMode(ENetMode NewNetMode)
{
    NetMode = NewNetMode;
}

bool World::IsStandalone() const
{
    return NetMode == ENetMode::Standalone;
}

bool World::IsListenServer() const
{
    return NetMode == ENetMode::ListenServer;
}

bool World::IsClient() const
{
    return NetMode == ENetMode::Client;
}

bool World::IsServer() const
{
    return IsStandalone() || IsListenServer();
}

FNetworkActorId World::AllocateNetworkActorId()
{
    if (!IsServer()) {
        return 0;
    }

    return NextNetworkActorId++;
}

bool World::ServerTravel(FNetworkSceneId SceneId)
{
    if (!IsServer() || !SceneManager::GetInstance().IsSceneRegistered(SceneId)) {
        return false;
    }

    if (IsListenServer() && ReplicationSystem) {
        ReplicationSystem->BroadcastServerTravel(SceneId);
    }

    return SceneManager::GetInstance().OpenSceneById(SceneId, GetNetMode());
}
