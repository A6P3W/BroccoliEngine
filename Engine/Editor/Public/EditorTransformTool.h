#pragma once

#include <cstdint>

#include "PhysicsQuery3D.h"
#include "UMath.h"

class AActor;

enum class EActorAction { Select, Move, Rotate, Scale };

enum class EGizmoHandle3D : uint8_t {
  None,
  X,
  Y,
  Z,
  XY,
  XZ,
  YZ,
  Center,
};

class EditorTransformTool {
 public:
  void Begin(AActor* Actor, EActorAction Action);
  void Update(const FVector2D& Delta, const FVector2D& MouseWorldPosition);
  EGizmoHandle3D HitTest3D(
      AActor* Actor, EActorAction Action, const FPhysicsRay3D& Ray, const FVector3D& CameraLocation
  ) const;
  bool Begin3D(
      AActor* Actor,
      EActorAction Action,
      EGizmoHandle3D Handle,
      const FPhysicsRay3D& Ray,
      const FVector3D& CameraDirection,
      const FVector2D& MousePosition,
      const FVector3D& CameraLocation
  );
  void Update3D(const FPhysicsRay3D& Ray, const FVector2D& MousePosition);
  void Draw3D(AActor* Actor, EActorAction Action, const FVector3D& CameraLocation) const;
  void End();

  bool IsActive() const { return TargetActor != nullptr; }

 private:
  AActor* TargetActor = nullptr;
  EActorAction Action = EActorAction::Select;
  EGizmoHandle3D Handle3D = EGizmoHandle3D::None;
  FTransform3D StartTransform3D;
  FVector3D StartIntersection3D;
  FVector3D DragPlaneNormal3D;
  FVector2D StartMousePosition3D;
  float GizmoSize3D = 1.0f;
};
