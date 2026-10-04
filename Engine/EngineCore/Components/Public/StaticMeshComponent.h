#pragma once

#include "Color.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MStaticMeshComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MStaticMeshComponent)

  void SetModel(int NewModelHandle) { ModelHandle = NewModelHandle; }
  int GetModel() const { return ModelHandle; }
  void SetMaterial(int NewMaterialHandle) { MaterialHandle = NewMaterialHandle; }
  int GetMaterial() const { return MaterialHandle; }
  void SetTint(const FColor& NewTint) { Tint = NewTint; }
  const FColor& GetTint() const { return Tint; }
  void Draw() override;

 private:
  int ModelHandle = 0;
  int MaterialHandle = 0;
  FColor Tint = FColor::White;
};
