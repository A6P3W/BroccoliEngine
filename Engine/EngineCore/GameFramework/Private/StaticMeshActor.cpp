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
  const FPath OldValue = ModelPath;
  ModelPath = Path;
  if (ModelPath == OldValue) return;
  OnModelPathChanged(OldValue);
}

void AStaticMeshActor::OnModelPathChanged(FPath OldValue) {
  (void)OldValue;
  if (StaticMeshComponent == nullptr) {
    return;
  }

  if (ModelPath.Empty()) {
    StaticMeshComponent->SetModel(0);
    return;
  }

  const int ModelHandle = ResourceManager::GetInstance().LoadResourceModel(ModelPath.String());
  if (ResourceManager::GetInstance().IsModelValid(ModelHandle)) {
    StaticMeshComponent->SetModel(ModelHandle);
  } else {
    StaticMeshComponent->SetModel(0);
  }
}

const FPath& AStaticMeshActor::GetModelPath() const { return ModelPath; }

void AStaticMeshActor::BeginPlay() {
  AActor::BeginPlay();
  if (!ModelPath.Empty()) OnModelPathChanged(ModelPath);
}
