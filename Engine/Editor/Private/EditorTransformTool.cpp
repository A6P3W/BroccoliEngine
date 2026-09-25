#include "EditorTransformTool.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Actor.h"
#include "RenderSystem.h"

namespace {
constexpr float AxisLength = 1.0f;
constexpr float AxisThickness = 0.08f;
constexpr float PlaneOffset = 0.28f;
constexpr float PlaneSize = 0.24f;
constexpr float PlaneThickness = 0.045f;
constexpr float CenterSize = 0.18f;
constexpr float MinimumScale = 0.01f;
constexpr float CameraSizeFactor = 0.1f;
constexpr float MinimumGizmoSize = 0.35f;

struct FGizmoBox3D {
  EGizmoHandle3D Handle = EGizmoHandle3D::None;
  FTransform3D Transform;
  FColor Color;
};

float GetGizmoSize(const AActor* Actor, const FVector3D& CameraLocation) {
  if (Actor == nullptr) return MinimumGizmoSize;
  return (std::max)((Actor->GetActorLocation3D() - CameraLocation).Size() * CameraSizeFactor,
                    MinimumGizmoSize);
}

FGizmoBox3D MakeBox(
    EGizmoHandle3D Handle, const FVector3D& Center, const FVector3D& Scale, const FColor& Color
) {
  return {Handle, {Center, FQuaternion::Identity(), {Scale.X, Scale.Y, Scale.Z}}, Color};
}

std::array<FGizmoBox3D, 7> BuildGizmoBoxes(
    const FVector3D& Origin, float Size, EActorAction Action
) {
  const float Length = AxisLength * Size;
  const float Thickness = AxisThickness * Size;
  const float Offset = PlaneOffset * Size;
  const float Plane = PlaneSize * Size;
  const float Thin = PlaneThickness * Size;
  const float Center = CenterSize * Size;
  const uint8_t CenterAlpha = Action == EActorAction::Scale ? 255 : 0;
  return {
      MakeBox(
          EGizmoHandle3D::X,
          Origin + FVector3D{Length * 0.5f, 0.0f, 0.0f},
          {Length, Thickness, Thickness},
          {230, 55, 55, 255}
      ),
      MakeBox(
          EGizmoHandle3D::Y,
          Origin + FVector3D{0.0f, Length * 0.5f, 0.0f},
          {Thickness, Length, Thickness},
          {65, 210, 90, 255}
      ),
      MakeBox(
          EGizmoHandle3D::Z,
          Origin + FVector3D{0.0f, 0.0f, Length * 0.5f},
          {Thickness, Thickness, Length},
          {65, 110, 235, 255}
      ),
      MakeBox(
          EGizmoHandle3D::XY,
          Origin + FVector3D{Offset, Offset, 0.0f},
          {Plane, Plane, Thin},
          {230, 210, 65, 210}
      ),
      MakeBox(
          EGizmoHandle3D::XZ,
          Origin + FVector3D{Offset, 0.0f, Offset},
          {Plane, Thin, Plane},
          {215, 65, 220, 210}
      ),
      MakeBox(
          EGizmoHandle3D::YZ,
          Origin + FVector3D{0.0f, Offset, Offset},
          {Thin, Plane, Plane},
          {65, 210, 220, 210}
      ),
      MakeBox(
          EGizmoHandle3D::Center, Origin, {Center, Center, Center}, {235, 235, 235, CenterAlpha}
      ),
  };
}

bool IntersectRayBox(const FPhysicsRay3D& Ray, const FTransform3D& Box, float& OutDistance) {
  const FVector3D HalfExtent{Box.Scale.X * 0.5f, Box.Scale.Y * 0.5f, Box.Scale.Z * 0.5f};
  const FVector3D Minimum = Box.Location - HalfExtent;
  const FVector3D Maximum = Box.Location + HalfExtent;
  float MinimumDistance = 0.0f;
  float MaximumDistance = Ray.MaxDistance;
  const std::array<float, 3> Origins{Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z};
  const std::array<float, 3> Directions{Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z};
  const std::array<float, 3> Minimums{Minimum.X, Minimum.Y, Minimum.Z};
  const std::array<float, 3> Maximums{Maximum.X, Maximum.Y, Maximum.Z};
  for (size_t Index = 0; Index < Origins.size(); ++Index) {
    if (std::abs(Directions[Index]) < 1e-6f) {
      if (Origins[Index] < Minimums[Index] || Origins[Index] > Maximums[Index]) return false;
      continue;
    }
    float Near = (Minimums[Index] - Origins[Index]) / Directions[Index];
    float Far = (Maximums[Index] - Origins[Index]) / Directions[Index];
    if (Near > Far) std::swap(Near, Far);
    MinimumDistance = (std::max)(MinimumDistance, Near);
    MaximumDistance = (std::min)(MaximumDistance, Far);
    if (MinimumDistance > MaximumDistance) return false;
  }
  OutDistance = MinimumDistance;
  return true;
}

FVector3D AxisForHandle(EGizmoHandle3D Handle) {
  if (Handle == EGizmoHandle3D::X) return {1.0f, 0.0f, 0.0f};
  if (Handle == EGizmoHandle3D::Y) return {0.0f, 1.0f, 0.0f};
  return {0.0f, 0.0f, 1.0f};
}

FVector3D PlaneNormalForHandle(EGizmoHandle3D Handle) {
  if (Handle == EGizmoHandle3D::XY) return {0.0f, 0.0f, 1.0f};
  if (Handle == EGizmoHandle3D::XZ) return {0.0f, 1.0f, 0.0f};
  return {1.0f, 0.0f, 0.0f};
}

bool IntersectRayPlane(
    const FPhysicsRay3D& Ray,
    const FVector3D& PlanePoint,
    const FVector3D& PlaneNormal,
    FVector3D& OutIntersection
) {
  const float Denominator = Ray.Direction.Dot(PlaneNormal);
  if (std::abs(Denominator) < 1e-5f) return false;
  const float Distance = (PlanePoint - Ray.Origin).Dot(PlaneNormal) / Denominator;
  if (Distance < 0.0f || Distance > Ray.MaxDistance) return false;
  OutIntersection = Ray.Origin + Ray.Direction * Distance;
  return true;
}

FVector3D BuildAxisDragPlaneNormal(const FVector3D& Axis, const FVector3D& CameraDirection) {
  const FVector3D Side = CameraDirection.Cross(Axis);
  if (Side.SizeSquared() > 1e-6f) return Axis.Cross(Side).Normalize();
  const FVector3D Fallback =
      std::abs(Axis.Y) < 0.9f ? FVector3D{0.0f, 1.0f, 0.0f} : FVector3D{1.0f, 0.0f, 0.0f};
  return Axis.Cross(Fallback).Cross(Axis).Normalize();
}
}  // namespace

void EditorTransformTool::Begin(AActor* Actor, EActorAction InAction) {
  TargetActor = Actor;
  Action = InAction;
}

void EditorTransformTool::Update(const FVector2D& Delta, const FVector2D& MouseWorldPosition) {
  if (TargetActor == nullptr || TargetActor->IsPendingDestroy()) {
    End();
    return;
  }

  switch (Action) {
    case EActorAction::Select:
      break;
    case EActorAction::Move:
      TargetActor->SetActorLocation(MouseWorldPosition);
      break;
    case EActorAction::Rotate:
      TargetActor->AddActorRotation(FRotator(Delta.X * 0.25f));
      break;
    case EActorAction::Scale:
      TargetActor->SetActorScale(TargetActor->GetActorScale() * (1.0f + Delta.X * 0.001f));
      break;
  }
}

EGizmoHandle3D EditorTransformTool::HitTest3D(
    AActor* Actor, EActorAction InAction, const FPhysicsRay3D& Ray, const FVector3D& CameraLocation
) const {
  if (Actor == nullptr || Actor->IsPendingDestroy() ||
      (InAction != EActorAction::Move && InAction != EActorAction::Scale)) {
    return EGizmoHandle3D::None;
  }
  const auto Boxes =
      BuildGizmoBoxes(Actor->GetActorLocation3D(), GetGizmoSize(Actor, CameraLocation), InAction);
  constexpr std::array<EGizmoHandle3D, 7> Priority{
      EGizmoHandle3D::Center,
      EGizmoHandle3D::XY,
      EGizmoHandle3D::XZ,
      EGizmoHandle3D::YZ,
      EGizmoHandle3D::X,
      EGizmoHandle3D::Y,
      EGizmoHandle3D::Z,
  };
  for (EGizmoHandle3D Candidate : Priority) {
    if (Candidate == EGizmoHandle3D::Center && InAction != EActorAction::Scale) continue;
    for (const FGizmoBox3D& Box : Boxes) {
      if (Box.Handle != Candidate) continue;
      float Distance = std::numeric_limits<float>::max();
      if (IntersectRayBox(Ray, Box.Transform, Distance)) return Candidate;
    }
  }
  return EGizmoHandle3D::None;
}

bool EditorTransformTool::Begin3D(
    AActor* Actor,
    EActorAction InAction,
    EGizmoHandle3D InHandle,
    const FPhysicsRay3D& Ray,
    const FVector3D& CameraDirection,
    const FVector2D& MousePosition,
    const FVector3D& CameraLocation
) {
  if (Actor == nullptr || Actor->IsPendingDestroy() || InHandle == EGizmoHandle3D::None ||
      (InAction != EActorAction::Move && InAction != EActorAction::Scale)) {
    return false;
  }
  TargetActor = Actor;
  Action = InAction;
  Handle3D = InHandle;
  StartTransform3D = Actor->GetActorTransform3D();
  StartMousePosition3D = MousePosition;
  GizmoSize3D = GetGizmoSize(Actor, CameraLocation);
  if (Handle3D == EGizmoHandle3D::Center) return true;
  if (Handle3D == EGizmoHandle3D::X || Handle3D == EGizmoHandle3D::Y ||
      Handle3D == EGizmoHandle3D::Z) {
    DragPlaneNormal3D = BuildAxisDragPlaneNormal(AxisForHandle(Handle3D), CameraDirection);
  } else {
    DragPlaneNormal3D = PlaneNormalForHandle(Handle3D);
  }
  if (!IntersectRayPlane(Ray, StartTransform3D.Location, DragPlaneNormal3D, StartIntersection3D)) {
    End();
    return false;
  }
  return true;
}

void EditorTransformTool::Update3D(const FPhysicsRay3D& Ray, const FVector2D& MousePosition) {
  if (TargetActor == nullptr || TargetActor->IsPendingDestroy()) {
    End();
    return;
  }
  if (Handle3D == EGizmoHandle3D::Center) {
    const float Factor = (std::max)(1.0f + (MousePosition.X - StartMousePosition3D.X -
                                            (MousePosition.Y - StartMousePosition3D.Y)) *
                                               0.01f,
                                    MinimumScale);
    TargetActor->SetActorScale3D({
        (std::max)(StartTransform3D.Scale.X * Factor, MinimumScale),
        (std::max)(StartTransform3D.Scale.Y * Factor, MinimumScale),
        (std::max)(StartTransform3D.Scale.Z * Factor, MinimumScale),
    });
    return;
  }
  FVector3D CurrentIntersection;
  if (!IntersectRayPlane(Ray, StartTransform3D.Location, DragPlaneNormal3D, CurrentIntersection)) {
    return;
  }
  const FVector3D Delta = CurrentIntersection - StartIntersection3D;
  FVector3D ConstrainedDelta;
  if (Handle3D == EGizmoHandle3D::X || Handle3D == EGizmoHandle3D::Y ||
      Handle3D == EGizmoHandle3D::Z) {
    const FVector3D Axis = AxisForHandle(Handle3D);
    ConstrainedDelta = Axis * Delta.Dot(Axis);
  } else if (Handle3D == EGizmoHandle3D::XY) {
    ConstrainedDelta = {Delta.X, Delta.Y, 0.0f};
  } else if (Handle3D == EGizmoHandle3D::XZ) {
    ConstrainedDelta = {Delta.X, 0.0f, Delta.Z};
  } else if (Handle3D == EGizmoHandle3D::YZ) {
    ConstrainedDelta = {0.0f, Delta.Y, Delta.Z};
  }
  if (Action == EActorAction::Move) {
    TargetActor->SetActorLocation3D(StartTransform3D.Location + ConstrainedDelta);
    return;
  }
  FScale3D Scale = StartTransform3D.Scale;
  Scale.X = (std::max)(Scale.X + ConstrainedDelta.X / GizmoSize3D, MinimumScale);
  Scale.Y = (std::max)(Scale.Y + ConstrainedDelta.Y / GizmoSize3D, MinimumScale);
  Scale.Z = (std::max)(Scale.Z + ConstrainedDelta.Z / GizmoSize3D, MinimumScale);
  TargetActor->SetActorScale3D(Scale);
}

void EditorTransformTool::Draw3D(
    AActor* Actor, EActorAction InAction, const FVector3D& CameraLocation
) const {
  if (Actor == nullptr || Actor->IsPendingDestroy() ||
      (InAction != EActorAction::Move && InAction != EActorAction::Scale)) {
    return;
  }
  const auto Boxes =
      BuildGizmoBoxes(Actor->GetActorLocation3D(), GetGizmoSize(Actor, CameraLocation), InAction);
  for (const FGizmoBox3D& Box : Boxes) {
    if (Box.Handle == EGizmoHandle3D::Center && InAction != EActorAction::Scale) continue;
    RenderSystem::GetInstance().SubmitCube(Box.Transform, Box.Color, true, ERenderLayer3D::Overlay);
  }
}

void EditorTransformTool::End() {
  TargetActor = nullptr;
  Action = EActorAction::Select;
  Handle3D = EGizmoHandle3D::None;
}
