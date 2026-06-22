#include "World.h"

#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "CameraComponent.h"
#include "World.h"
World::World()
{
    ObjectManager = std::make_unique<MObjectManager>();
    CollisionSystem = std::make_unique<MCollisionSystem>();
    SoundManager = std::make_unique<MSoundManager>();
    TimerManager = std::make_unique<MTimerManager>();
    
	ObjectManager->SetWorld(this);
}

World::~World()
{
	bTrendingDown = true;
}

void World::Update(float DeltaTime)
{

    if (ObjectManager)ObjectManager->Update(DeltaTime);
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
