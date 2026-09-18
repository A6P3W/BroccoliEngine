#pragma once

#include "IEditorPanel.h"
#include "UMath.h"

class AActor;

class InspectorPanel final : public IEditorPanel {
 public:
  std::string_view GetId() const override { return "Inspector"; }
  std::string_view GetTitle() const override { return "Inspector"; }

 protected:
  void DrawContents(EditorContext& Context) override;

 private:
  void SynchronizeRotation(AActor* Actor);

  AActor* RotationActor = nullptr;
  FQuaternion LastAppliedRotation = FQuaternion::Identity();
  FRotator3D CachedRotation;
  bool bHasCachedRotation = false;
};
