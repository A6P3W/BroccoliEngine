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
    m_ObjectManager = std::make_unique<ObjectManager>();
    m_CollisionSystem = std::make_unique<CollisionSystem>();
    m_SoundManager = std::make_unique<SoundManager>();
    m_TimerManager = std::make_unique<TimerManager>();
    
	m_ObjectManager->SetWorld(this);
}

World::~World()
{
	bTrendingDown = true;
}

void World::Update(float DeltaTime)
{

    if (m_ObjectManager)m_ObjectManager->Update(DeltaTime);
    if (bSimulating) {
        if (m_TimerManager)m_TimerManager->Update(DeltaTime);
        if (m_CollisionSystem)m_CollisionSystem->CheckCollisions();
    }
}

void World::Draw()
{
    if (m_ObjectManager) m_ObjectManager->Draw();
}

void World::SetGameMode(AGameModeBase* mode)
{
    m_GameMode = mode;
}
