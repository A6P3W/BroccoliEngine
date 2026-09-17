#include "SceneComponent.h"

#include <algorithm>

#include "Log.h"

struct MSceneComponent::Impl {
  MSceneComponent* ParentComponent = nullptr;
  std::vector<MSceneComponent*> ChildComponents;
  FTransform3D RelativeTransform;
  FTransform3D WorldTransform;
  bool Visible = true;
  bool TransformDirty = true;
  bool GridDirty = true;
};

MSceneComponent::MSceneComponent() : ImplPtr(new Impl()) {}
MSceneComponent::~MSceneComponent() { delete ImplPtr; }
MSceneComponent* MSceneComponent::GetParentComponent() const { return ImplPtr->ParentComponent; }
bool MSceneComponent::IsVisible() const { return ImplPtr->Visible; }
bool MSceneComponent::bGridDirty() const { return ImplPtr->GridDirty; }
void MSceneComponent::SetGridClean() { ImplPtr->GridDirty = false; }
const std::vector<MSceneComponent*>& MSceneComponent::GetChildComponents() const {
  return ImplPtr->ChildComponents;
}
void MSceneComponent::OnUpdate(float DeltaTime) { (void)DeltaTime; }
void MSceneComponent::Draw() {}
void MSceneComponent::OnMessage(const std::string& Message) { (void)Message; }

void MSceneComponent::OnComponentDestroy() {
  const auto Children = ImplPtr->ChildComponents;
  for (auto* Child : Children)
    if (Child != nullptr) Child->DestroyComponent();
  ImplPtr->ChildComponents.clear();
  if (ImplPtr->ParentComponent != nullptr)
    std::erase(ImplPtr->ParentComponent->ImplPtr->ChildComponents, this);
}
void MSceneComponent::AttachToComponent(MSceneComponent* Parent) {
  (void)AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
}
bool MSceneComponent::AttachToComponent(
    MSceneComponent* Parent, const FAttachmentTransformRules& Rules
) {
  if (Parent == this) return false;
  for (auto* Current = Parent; Current != nullptr; Current = Current->GetParentComponent())
    if (Current == this) return false;
  const FTransform3D OldWorld = GetWorldTransform3D();
  FTransform3D NewRelative = GetRelativeTransform3D();
  if (Parent != nullptr &&
      (Rules.LocationRule == EAttachmentRule::KeepWorld ||
       Rules.ScaleRule == EAttachmentRule::KeepWorld) &&
      Parent->GetWorldScale3D().IsNearlyZero())
    return false;
  if (Rules.LocationRule == EAttachmentRule::KeepWorld)
    NewRelative.Location =
        Parent ? Parent->GetWorldTransform3D().InverseTransformPosition(OldWorld.Location)
               : OldWorld.Location;
  else if (Rules.LocationRule == EAttachmentRule::SnapToTarget)
    NewRelative.Location = FVector3D::ZeroVector();
  if (Rules.RotationRule == EAttachmentRule::KeepWorld)
    NewRelative.Rotation =
        Parent ? (Parent->GetWorldRotation3D().Inverse() * OldWorld.Rotation).Normalize()
               : OldWorld.Rotation;
  else if (Rules.RotationRule == EAttachmentRule::SnapToTarget)
    NewRelative.Rotation = FQuaternion::Identity();
  if (Rules.ScaleRule == EAttachmentRule::KeepWorld)
    NewRelative.Scale = Parent ? OldWorld.Scale / Parent->GetWorldScale3D() : OldWorld.Scale;
  else if (Rules.ScaleRule == EAttachmentRule::SnapToTarget)
    NewRelative.Scale = FScale3D{};
  if (ImplPtr->ParentComponent != Parent) {
    if (ImplPtr->ParentComponent)
      std::erase(ImplPtr->ParentComponent->ImplPtr->ChildComponents, this);
    ImplPtr->ParentComponent = Parent;
    if (Parent &&
        std::find(
            Parent->ImplPtr->ChildComponents.begin(), Parent->ImplPtr->ChildComponents.end(), this
        ) == Parent->ImplPtr->ChildComponents.end())
      Parent->ImplPtr->ChildComponents.push_back(this);
  }
  ImplPtr->RelativeTransform = NewRelative;
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::SetWorldLocation3D(const FVector3D& Location) {
  if (!ImplPtr->ParentComponent)
    ImplPtr->RelativeTransform.Location = Location;
  else {
    const auto Parent = ImplPtr->ParentComponent->GetWorldTransform3D();
    if (Parent.Scale.IsNearlyZero()) return false;
    ImplPtr->RelativeTransform.Location = Parent.InverseTransformPosition(Location);
  }
  MakeTransformDirty();
  return true;
}
bool MSceneComponent::SetRelativeLocation3D(const FVector3D& Location) {
  ImplPtr->RelativeTransform.Location = Location;
  MakeTransformDirty();
  return true;
}
FVector3D MSceneComponent::GetWorldLocation3D() const { return GetWorldTransform3D().Location; }
FVector3D MSceneComponent::GetRelativeLocation3D() const {
  return ImplPtr->RelativeTransform.Location;
}
bool MSceneComponent::SetWorldRotation3D(const FQuaternion& Rotation) {
  ImplPtr->RelativeTransform.Rotation =
      ImplPtr->ParentComponent
          ? (ImplPtr->ParentComponent->GetWorldRotation3D().Inverse() * Rotation).Normalize()
          : Rotation.Normalize();
  MakeTransformDirty();
  return true;
}
bool MSceneComponent::SetRelativeRotation3D(const FQuaternion& Rotation) {
  ImplPtr->RelativeTransform.Rotation = Rotation.Normalize();
  MakeTransformDirty();
  return true;
}
FQuaternion MSceneComponent::GetWorldRotation3D() const { return GetWorldTransform3D().Rotation; }
FQuaternion MSceneComponent::GetRelativeRotation3D() const {
  return ImplPtr->RelativeTransform.Rotation;
}
bool MSceneComponent::SetWorldScale3D(const FScale3D& Scale) {
  if (ImplPtr->ParentComponent) {
    const auto ParentScale = ImplPtr->ParentComponent->GetWorldScale3D();
    if (ParentScale.IsNearlyZero()) return false;
    ImplPtr->RelativeTransform.Scale = Scale / ParentScale;
  } else
    ImplPtr->RelativeTransform.Scale = Scale;
  MakeTransformDirty();
  return true;
}
bool MSceneComponent::SetRelativeScale3D(const FScale3D& Scale) {
  ImplPtr->RelativeTransform.Scale = Scale;
  MakeTransformDirty();
  return true;
}
FScale3D MSceneComponent::GetWorldScale3D() const { return GetWorldTransform3D().Scale; }
FScale3D MSceneComponent::GetRelativeScale3D() const { return ImplPtr->RelativeTransform.Scale; }
FTransform3D MSceneComponent::GetWorldTransform3D() const {
  UpdateTransform();
  return ImplPtr->WorldTransform;
}
FTransform3D MSceneComponent::GetRelativeTransform3D() const { return ImplPtr->RelativeTransform; }

bool MSceneComponent::SetWorldLocation(const FVector2D& Location) {
  const auto Old = GetWorldLocation3D();
  return SetWorldLocation3D({Location.X, Location.Y, Old.Z});
}
bool MSceneComponent::SetRelativeLocation(const FVector2D& Location) {
  const auto Old = GetRelativeLocation3D();
  return SetRelativeLocation3D({Location.X, Location.Y, Old.Z});
}
bool MSceneComponent::AddWorldOffset(const FVector2D& Offset) {
  const auto Position = GetWorldLocation3D();
  return SetWorldLocation3D({Position.X + Offset.X, Position.Y + Offset.Y, Position.Z});
}
bool MSceneComponent::AddLocalOffset(const FVector2D& Offset) {
  return SetWorldLocation3D(
      GetWorldLocation3D() + GetWorldRotation3D().RotateVector({Offset.X, Offset.Y, 0.0f})
  );
}
FVector2D MSceneComponent::GetWorldLocation() const {
  const auto Position = GetWorldLocation3D();
  return {Position.X, Position.Y};
}
FVector2D MSceneComponent::GetRelativeLocation() const {
  const auto Position = GetRelativeLocation3D();
  return {Position.X, Position.Y};
}
bool MSceneComponent::SetWorldRotation(FRotator Rotation) {
  const auto Euler = GetWorldRotation3D().ToRotator();
  return SetWorldRotation3D(FQuaternion::FromRotator({Euler.Pitch, Rotation.Rotation, Euler.Roll}));
}
bool MSceneComponent::AddWorldRotation(FRotator Rotation) {
  return SetWorldRotation(FRotator(GetWorldRotation().Rotation + Rotation.Rotation));
}
FRotator MSceneComponent::GetWorldRotation() const {
  return FRotator(GetWorldRotation3D().ToRotator().Yaw);
}
FRotator MSceneComponent::GetRelativeRotation() const {
  return FRotator(GetRelativeRotation3D().ToRotator().Yaw);
}
bool MSceneComponent::SetRelativeScale(FScale Scale) {
  return SetRelativeScale3D(FScale3D(Scale.Scale));
}
FScale MSceneComponent::GetRelativeScale() const { return FScale(GetRelativeScale3D().X); }
bool MSceneComponent::SetWorldScale(FScale Scale) { return SetWorldScale3D(FScale3D(Scale.Scale)); }
FScale MSceneComponent::GetWorldScale() const { return FScale(GetWorldScale3D().X); }
void MSceneComponent::SetVisibility(bool Visible) {
  ImplPtr->Visible = Visible;
  for (auto* Child : ImplPtr->ChildComponents) Child->SetVisibility(Visible);
}
void MSceneComponent::MakeTransformDirty() {
  if (ImplPtr->TransformDirty) return;
  ImplPtr->TransformDirty = true;
  ImplPtr->GridDirty = true;
  for (auto* Child : ImplPtr->ChildComponents) Child->MakeTransformDirty();
}
void MSceneComponent::UpdateTransform() const {
  if (!ImplPtr->TransformDirty) return;
  ImplPtr->WorldTransform =
      ImplPtr->ParentComponent
          ? FTransform3D::Combine(
                ImplPtr->ParentComponent->GetWorldTransform3D(), ImplPtr->RelativeTransform
            )
          : ImplPtr->RelativeTransform;
  ImplPtr->TransformDirty = false;
}
