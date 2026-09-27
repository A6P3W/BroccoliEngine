#include "StaticMeshActor.h"

#include "ResourceManager.h"
#include "StaticMeshComponent.h"

REGISTER_ACTOR(AStaticMeshActor);

AStaticMeshActor::AStaticMeshActor() {
  StaticMeshComponent = NewObject<MStaticMeshComponent>(this);
  SetRootComponent(StaticMeshComponent);
  if (StaticMeshComponent) {
    StaticMeshComponent->RegisterComponent();
  }
}

AStaticMeshActor::~AStaticMeshActor() = default;

void AStaticMeshActor::SetModelPath(const FPath& Path) {
  ModelPath = Path;
  if (StaticMeshComponent == nullptr) {
    return;
  }

  if (ModelPath.Empty()) {
    StaticMeshComponent->SetModel(0);
    return;
  }

  const int ModelHandle = ResourceManager::GetInstance().LoadResourceModel(ModelPath);
  if (ResourceManager::GetInstance().IsModelValid(ModelHandle)) {
    StaticMeshComponent->SetModel(ModelHandle);
  } else {
    StaticMeshComponent->SetModel(0);
  }
}

void AStaticMeshActor::OnModelPathChanged(FPath OldValue) {
  (void)OldValue;
  SetModelPath(ModelPath);
}

const FPath& AStaticMeshActor::GetModelPath() const { return ModelPath; }

void AStaticMeshActor::BeginPlay() {
  AActor::BeginPlay();
  if (!ModelPath.Empty()) SetModelPath(ModelPath);
}
