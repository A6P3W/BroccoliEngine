#include "CollisionTestActors.h"

#include "CircleCollisionComponent.h"
#include "CollisionComponent.h"
#include "LineCollisionComponent.h"
#include "Log.h"
#include "RectangleCollisionComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ACollisionTestRectangleActor)
REGISTER_ACTOR(ACollisionTestBlockRectangleActor)
REGISTER_ACTOR(ACollisionTestCircleActor)
REGISTER_ACTOR(ACollisionTestLineActor)

void ACollisionTestActorBase::ConfigureCollision(
    MCollisionComponent* Collision, const char* ShapeName
) {
  ShapeLabel = ShapeName;
  Collision->SetCollisionType(ECollisionType::Overlap);
  Collision->SetStatic(false);
  Collision->AttachToComponent(GetRootComponent());
  Collision->RegisterComponent();
}

void ACollisionTestActorBase::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);
  ElapsedSeconds += DeltaTime;
  AddActorRotation(FRotator(25.0f * DeltaTime));
  DRAW_WORLD_LOG("CollisionShape", 0.1f, this, "{}: overlap / rotating", ShapeLabel);
  DRAW_SCREEN_LOG(
      "CollisionHelp", 0.1f, "CollisionTest: collision shapes are debug-drawn; Escape LevelStarter"
  );
}

void ACollisionTestActorBase::BeginOverlap(AActor* OtherActor) {
  M_LOG("{} BeginOverlap with {}", ShapeLabel, OtherActor->GetActorClassName());
  DRAW_WORLD_LOG("CollisionBegin", 1.0f, this, "BeginOverlap: {}", OtherActor->GetActorClassName());
}

void ACollisionTestActorBase::EndOverlap(AActor* OtherActor) {
  M_LOG("{} EndOverlap with {}", ShapeLabel, OtherActor->GetActorClassName());
  DRAW_WORLD_LOG("CollisionEnd", 1.0f, this, "EndOverlap: {}", OtherActor->GetActorClassName());
}

ACollisionTestRectangleActor::ACollisionTestRectangleActor() {
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  Collision->SetSize(180.0f, 90.0f);
  ConfigureCollision(Collision, "Rectangle");
}

ACollisionTestBlockRectangleActor::ACollisionTestBlockRectangleActor() {
  auto* Collision = NewObject<MRectangleCollisionComponent>(this);
  Collision->SetSize(130.0f, 70.0f);
  ConfigureCollision(Collision, "Rectangle (Block)");
  Collision->SetCollisionType(ECollisionType::Block);
}
ACollisionTestCircleActor::ACollisionTestCircleActor() {
  auto* Collision = NewObject<MCircleCollisionComponent>(this);
  Collision->SetRadius(55.0f);
  ConfigureCollision(Collision, "Circle");
}

ACollisionTestLineActor::ACollisionTestLineActor() {
  auto* Collision = NewObject<MLineCollisionComponent>(this);
  Collision->SetLine({-90.0f, 0.0f}, {90.0f, 0.0f});
  ConfigureCollision(Collision, "Line");
}
