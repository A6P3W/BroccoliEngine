#pragma once

#include "Actor.h"

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
  void OnShadingModelChanged(int OldValue);
  void RebuildMaterial();
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AStaticMeshActor::OnModelPathChanged,
          .PathFilter = EPathFilter::Model
  )
  FPath ModelPath;
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AStaticMeshActor::OnShadingModelChanged,
          .Min = 0,
          .Max = 1,
          .EnumOptions = "Unlit|Lit"
  )
  int ShadingModel = 1;
};
