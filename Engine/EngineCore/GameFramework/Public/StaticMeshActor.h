#pragma once

#include "Actor.h"
#include "Color.h"

class MStaticMeshComponent;

class BROCCOLI_ENGINE_API AStaticMeshActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AStaticMeshActor);

  AStaticMeshActor();
  ~AStaticMeshActor() override;

  void SetModelPath(const FPath& Path);
  const FPath& GetModelPath() const;

 protected:
  void BeginPlay() override;

 private:
  MStaticMeshComponent* StaticMeshComponent = nullptr;
  void OnModelPathChanged(FPath OldValue);
  void OnMaterialChanged(int OldValue);
  void OnBaseColorChanged(FColor OldValue);
  void OnTexturePathChanged(FPath OldValue);
  void RebuildMaterial();
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AStaticMeshActor::OnModelPathChanged,
          .PathFilter = EPathFilter::Model
  )
  FPath ModelPath;
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AStaticMeshActor::OnMaterialChanged,
          .Min = 0,
          .Max = 1,
          .EnumOptions = "Unlit|Lit"
  )
  int ShadingModel = 1;
  EDITOR_PROPERTY(.OnEditorChanged = ^^AStaticMeshActor::OnBaseColorChanged)
  FColor BaseColor = FColor::White;
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AStaticMeshActor::OnTexturePathChanged,
          .PathFilter = EPathFilter::Image
  )
  FPath BaseColorTexturePath;
};
