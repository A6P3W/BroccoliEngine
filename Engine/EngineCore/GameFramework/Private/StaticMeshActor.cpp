#include "StaticMeshActor.h"

#include <stdexcept>

#include "Log.h"
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
  const auto VirtualPath = PathResolver::MakeVirtualPath(Path);
  if (!VirtualPath) {
    M_LOG(Warning, "Invalid model resource path: {}", Path);
    return;
  }
  const FPath OldValue = ModelPath;
  try {
    ModelPath = FPath(*VirtualPath);
  } catch (const std::invalid_argument&) {
    M_LOG(Warning, "Invalid model resource path: {}", Path);
    return;
  }
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

const std::string& AStaticMeshActor::GetModelPath() const { return ModelPath.String(); }

void AStaticMeshActor::BeginPlay() {
  AActor::BeginPlay();
  if (!ModelPath.Empty()) OnModelPathChanged(ModelPath);
}
