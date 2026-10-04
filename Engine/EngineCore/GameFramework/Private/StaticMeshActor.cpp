#include "StaticMeshActor.h"

#include "ResourceManager.h"
#include "StaticMeshComponent.h"

REGISTER_ACTOR(AStaticMeshActor);

AStaticMeshActor::AStaticMeshActor() {
  StaticMeshComponent = NewObject<MStaticMeshComponent>(this);
  SetRootComponent(StaticMeshComponent);
  if (StaticMeshComponent) {
    StaticMeshComponent->RegisterComponent();
    RebuildMaterial();
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

void AStaticMeshActor::RebuildMaterial() {
  if (StaticMeshComponent == nullptr) return;
  auto& Resources = ResourceManager::GetInstance();
  FMaterial3DDesc Descriptor;
  Descriptor.ShadingModel = ShadingModel == 0 ? EShadingModel3D::Unlit : EShadingModel3D::Lit;
  Descriptor.BaseColor = BaseColor;
  if (!BaseColorTexturePath.Empty())
    Descriptor.BaseColorTextureHandle = Resources.LoadResourceGraph(BaseColorTexturePath);
  StaticMeshComponent->SetMaterial(Resources.CreateMaterial3D(Descriptor));
}

void AStaticMeshActor::OnMaterialChanged(int OldValue) {
  (void)OldValue;
  RebuildMaterial();
}

void AStaticMeshActor::OnBaseColorChanged(FColor OldValue) {
  (void)OldValue;
  RebuildMaterial();
}

void AStaticMeshActor::OnTexturePathChanged(FPath OldValue) {
  (void)OldValue;
  RebuildMaterial();
}

const FPath& AStaticMeshActor::GetModelPath() const { return ModelPath; }

void AStaticMeshActor::BeginPlay() {
  AActor::BeginPlay();
  if (!ModelPath.Empty()) SetModelPath(ModelPath);
}
