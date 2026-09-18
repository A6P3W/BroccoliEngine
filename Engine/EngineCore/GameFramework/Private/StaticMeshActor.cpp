#include "StaticMeshActor.h"

#include "PathResolver.h"
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

void AStaticMeshActor::SetModelPath(const std::string& Path) {
  ModelPath = PathResolver::SanitizeResourcePath(Path);
  if (StaticMeshComponent == nullptr || ModelPath.empty()) {
    return;
  }

  const int ModelHandle = ResourceManager::GetInstance().LoadResourceModel(ModelPath);
  if (ResourceManager::GetInstance().IsModelValid(ModelHandle)) {
    StaticMeshComponent->SetModel(ModelHandle);
  }
}

const std::string& AStaticMeshActor::GetModelPath() const { return ModelPath; }

void AStaticMeshActor::BeginPlay() {
  AActor::BeginPlay();
  if (!ModelPath.empty()) {
    SetModelPath(ModelPath);
  }
}
