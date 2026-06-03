#include "GameModeBase.h"
#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "CameraComponent.h"

AGameModeBase::AGameModeBase()
{
    m_ObjectManager = std::make_unique<ObjectManager>();
    m_CollisionSystem = std::make_unique<CollisionSystem>();
    m_TimerManager = std::make_unique<TimerManager>();


    auto camera = std::make_unique<MCameraComponent>();
    auto* camPtr = camera.get();
    this->AddComponent(std::move(camera)); 
    camPtr->SetActiveCamera();
}

void AGameModeBase::OnUpdate(float DeltaTime)
{
    if (m_TimerManager)m_TimerManager->Update(DeltaTime);
    if (m_ObjectManager)m_ObjectManager->Update(DeltaTime);
    if (m_CollisionSystem)m_CollisionSystem->CheckCollisions();
}

void AGameModeBase::Draw()
{
    if (m_ObjectManager) m_ObjectManager->Draw();
    AActor::Draw();
}

