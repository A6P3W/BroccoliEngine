#include "SceneComponent.h"

#include <algorithm>
#include <cmath>

#include "Actor.h"
#include "Log.h"
#include "UMath.h"

struct MSceneComponent::Impl {
  MSceneComponent* ParentComponent = nullptr;
  std::vector<MSceneComponent*> ChildComponents;
  FVector2D RelativeLocation;
  FRotator RelativeRotation;
  FScale RelativeScale;
  FVector2D WorldLocation;
  FRotator WorldRotation;
  FScale WorldScale;
  bool Visible = true;
  bool TransformDirty = true;
  bool GridDirty = true;
};
MSceneComponent::MSceneComponent() : ImplPtr(new Impl()) {}

MSceneComponent::~MSceneComponent() {
  delete ImplPtr;
}

MSceneComponent* MSceneComponent::GetParentComponent() const { return ImplPtr->ParentComponent; }

bool MSceneComponent::IsVisible() const { return ImplPtr->Visible; }

bool MSceneComponent::bGridDirty() const { return ImplPtr->GridDirty; }

void MSceneComponent::SetGridClean() { ImplPtr->GridDirty = true; }

const std::vector<MSceneComponent*>& MSceneComponent::GetChildComponents() const {
  return ImplPtr->ChildComponents;
}
void MSceneComponent::OnUpdate(float DeltaTime) {}

void MSceneComponent::Draw() {}

void MSceneComponent::OnMessage(const std::string& message) {}

void MSceneComponent::OnComponentDestroy() {
  const auto children = ImplPtr->ChildComponents;
  for (auto* child : children) {
    if (child != nullptr) {
      child->DestroyComponent();
    }
  }
  ImplPtr->ChildComponents.clear();

  if (ImplPtr->ParentComponent != nullptr) {
    std::erase(ImplPtr->ParentComponent->ImplPtr->ChildComponents, this);
    ImplPtr->ParentComponent = nullptr;
  }
}

void MSceneComponent::AttachToComponent(MSceneComponent* Parent) {
  (void)AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
}

bool MSceneComponent::AttachToComponent(
    MSceneComponent* Parent, const FAttachmentTransformRules& Rules
) {
  if (Parent == this) {
    M_LOG("AttachToComponent rejected: parent is self.");
    return false;
  }

  for (MSceneComponent* Current = Parent; Current != nullptr;
       Current = Current->GetParentComponent()) {
    if (Current == this) {
      M_LOG("AttachToComponent rejected: cyclic component hierarchy.");
      return false;
    }
  }

  const FVector2D OldRelativeLocation = GetRelativeLocation();
  const FRotator OldRelativeRotation = GetRelativeRotation();
  const FScale OldRelativeScale = GetRelativeScale();

  const FVector2D OldWorldLocation = GetWorldLocation();
  const FRotator OldWorldRotation = GetWorldRotation();
  const FScale OldWorldScale = GetWorldScale();

  FVector2D NewRelativeLocation = OldRelativeLocation;
  FRotator NewRelativeRotation = OldRelativeRotation;
  FScale NewRelativeScale = OldRelativeScale;

  if (Rules.LocationRule == EAttachmentRule::KeepWorld) {
    if (Parent == nullptr) {
      NewRelativeLocation = OldWorldLocation;
    } else {
      const FScale ParentScale = Parent->GetWorldScale();
      if (std::abs(ParentScale.Scale) < 1e-6f) {
        M_LOG("AttachToComponent rejected: parent scale is zero.");
        return false;
      }

      const FVector2D Difference = OldWorldLocation - Parent->GetWorldLocation();
      const FRotator InverseParentRotation(-Parent->GetWorldRotation().Rotation);
      NewRelativeLocation = Difference.RotateVector(InverseParentRotation) / ParentScale;
    }
  } else if (Rules.LocationRule == EAttachmentRule::SnapToTarget) {
    NewRelativeLocation = FVector2D::ZeroVector();
  }

  if (Rules.RotationRule == EAttachmentRule::KeepWorld) {
    NewRelativeRotation =
        Parent != nullptr ? OldWorldRotation - Parent->GetWorldRotation() : OldWorldRotation;
  } else if (Rules.RotationRule == EAttachmentRule::SnapToTarget) {
    NewRelativeRotation = FRotator(0.0f);
  }

  if (Rules.ScaleRule == EAttachmentRule::KeepWorld) {
    if (Parent == nullptr) {
      NewRelativeScale = OldWorldScale;
    } else {
      const FScale ParentScale = Parent->GetWorldScale();
      if (std::abs(ParentScale.Scale) < 1e-6f) {
        M_LOG("AttachToComponent rejected: parent scale is zero.");
        return false;
      }

      NewRelativeScale = OldWorldScale / ParentScale;
    }
  } else if (Rules.ScaleRule == EAttachmentRule::SnapToTarget) {
    NewRelativeScale = FScale(1.0f);
  }

  if (ImplPtr->ParentComponent != Parent) {
    if (ImplPtr->ParentComponent != nullptr) {
      std::erase(ImplPtr->ParentComponent->ImplPtr->ChildComponents, this);
    }

    ImplPtr->ParentComponent = Parent;

    if (ImplPtr->ParentComponent != nullptr &&
        std::find(
            ImplPtr->ParentComponent->ImplPtr->ChildComponents.begin(), ImplPtr->ParentComponent->ImplPtr->ChildComponents.end(), this
        ) == ImplPtr->ParentComponent->ImplPtr->ChildComponents.end()) {
      ImplPtr->ParentComponent->ImplPtr->ChildComponents.push_back(this);
    }
  }

  ImplPtr->RelativeLocation = NewRelativeLocation;
  ImplPtr->RelativeRotation = NewRelativeRotation;
  ImplPtr->RelativeScale = NewRelativeScale;

  MakeTransformDirty();
  return true;
}

bool MSceneComponent::SetWorldLocation(const FVector2D& NewWorldLocation) {
  if (!ImplPtr->ParentComponent) {
    // 親がいない（RootComponentである）場合、
    // ワールド座標はそのまま相対座標（ImplPtr->RelativeLocation）になる
    ImplPtr->RelativeLocation = NewWorldLocation;
  } else {
    const FScale ParentWorldScale = ImplPtr->ParentComponent->GetWorldScale();
    if (std::abs(ParentWorldScale.Scale) < 1e-6f) {
      return false;
    }

    // 親がいる場合、ワールド座標を親のローカル座標系に変換する必要がある
    FVector2D ParentWorldLoc = ImplPtr->ParentComponent->GetWorldLocation();
    float ParentWorldRotRad = UMath::DegToRad(ImplPtr->ParentComponent->GetWorldRotation().Rotation);

    // 1. 親との差分を取る
    float diffX = NewWorldLocation.X - ParentWorldLoc.X;
    float diffY = NewWorldLocation.Y - ParentWorldLoc.Y;

    // 2. 親の回転の「逆」をかけて、親から見た相対位置を出す
    // 逆回転行列: [ cos(-θ) -sin(-θ) ] = [  cosθ  sinθ ]
    //              [ sin(-θ)  cos(-θ) ]   [ -sinθ  cosθ ]
    float s = std::sin(ParentWorldRotRad);
    float c = std::cos(ParentWorldRotRad);

    ImplPtr->RelativeLocation.X = diffX * c + diffY * s;
    ImplPtr->RelativeLocation.Y = -diffX * s + diffY * c;
    ImplPtr->RelativeLocation /= ParentWorldScale;
  }
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::SetRelativeLocation(const FVector2D& NewRelativeLocation) {
  ImplPtr->RelativeLocation = NewRelativeLocation;
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::AddWorldOffset(const FVector2D& Offset) {
  FVector2D newWorldLocation = GetWorldLocation() + Offset;
  MakeTransformDirty();
  return SetWorldLocation(newWorldLocation);
}

bool MSceneComponent::AddLocalOffset(const FVector2D& Offset) {
  float worldRotRad = UMath::DegToRad(GetWorldRotation().Rotation);
  float s = std::sin(worldRotRad);
  float c = std::cos(worldRotRad);

  FVector2D worldOffset;
  worldOffset.X = Offset.X * c - Offset.Y * s;
  worldOffset.Y = Offset.X * s + Offset.Y * c;

  FVector2D newWorldLocation = GetWorldLocation() + worldOffset;
  MakeTransformDirty();
  return SetWorldLocation(newWorldLocation);
}

bool MSceneComponent::SetWorldRotation(FRotator NewRotation) {
  if (!ImplPtr->ParentComponent) {
    ImplPtr->RelativeRotation = NewRotation;
  } else {
    FRotator parentWorldRot = ImplPtr->ParentComponent->GetWorldRotation();
    ImplPtr->RelativeRotation = NewRotation - parentWorldRot;
  }
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::AddWorldRotation(FRotator DeltaRotation) {
  ImplPtr->RelativeRotation += DeltaRotation;
  MakeTransformDirty();
  return true;
}

FVector2D MSceneComponent::GetWorldLocation() const {
  UpdateTransform();
  return ImplPtr->WorldLocation;
}

FVector2D MSceneComponent::GetRelativeLocation() const { return ImplPtr->RelativeLocation; }

FRotator MSceneComponent::GetWorldRotation() const {
  UpdateTransform();
  return ImplPtr->WorldRotation;
}

FRotator MSceneComponent::GetRelativeRotation() const { return ImplPtr->RelativeRotation; }

bool MSceneComponent::SetRelativeScale(FScale NewScale) {
  ImplPtr->RelativeScale = NewScale;
  MakeTransformDirty();
  return true;
}

FScale MSceneComponent::GetRelativeScale() const { return ImplPtr->RelativeScale; }

bool MSceneComponent::SetWorldScale(FScale NewScale) {
  if (!ImplPtr->ParentComponent) {
    // 親がいなければワールドスケール = 相対スケール
    return SetRelativeScale(NewScale);
  }

  // 親がいる場合： 自分の相対 = 目標ワールド / 親のワールド
  FScale parentWorldScale = ImplPtr->ParentComponent->GetWorldScale();
  if (std::abs(parentWorldScale.Scale) < 1e-6f) return false;

  return SetRelativeScale(NewScale / parentWorldScale);
}

FScale MSceneComponent::GetWorldScale() const {
  UpdateTransform();
  return ImplPtr->WorldScale;
}

void MSceneComponent::SetVisibility(bool bNewVisibility) {
  ImplPtr->Visible = bNewVisibility;
  for (MSceneComponent* child : ImplPtr->ChildComponents) {
    child->SetVisibility(bNewVisibility);
  }
}

void MSceneComponent::MakeTransformDirty() {
  if (ImplPtr->TransformDirty) {
    return;
  }
  ImplPtr->TransformDirty = true;
  ImplPtr->GridDirty = true;
  for (MSceneComponent* child : ImplPtr->ChildComponents) {
    child->MakeTransformDirty();
  }
}

void MSceneComponent::UpdateTransform() const {
  if (!ImplPtr->TransformDirty) return;

  if (!ImplPtr->ParentComponent) {
    // 親がいない場合は相対値がそのままワールド値
    ImplPtr->WorldLocation = ImplPtr->RelativeLocation;
    ImplPtr->WorldRotation = ImplPtr->RelativeRotation;
    ImplPtr->WorldScale = ImplPtr->RelativeScale;
  } else {
    // 親の最新トランスフォームを先に確定させる（再帰）
    FScale pScale = ImplPtr->ParentComponent->GetWorldScale();
    FRotator pRot = ImplPtr->ParentComponent->GetWorldRotation();
    FVector2D pLoc = ImplPtr->ParentComponent->GetWorldLocation();

    // 1. スケールの計算
    ImplPtr->WorldScale = ImplPtr->RelativeScale * pScale;

    // 2. 回転の計算
    ImplPtr->WorldRotation = pRot + ImplPtr->RelativeRotation;

    // 3. 座標の計算（親の回転とスケールを考慮）
    float parentRad = UMath::DegToRad(pRot.Rotation);
    float s = std::sin(parentRad);
    float c = std::cos(parentRad);

    // 自分の相対座標に親のスケールを適用
    FVector2D scaledLocation = ImplPtr->RelativeLocation * pScale;

    // 親の回転に合わせて回転させる
    float rotatedX = scaledLocation.X * c - scaledLocation.Y * s;
    float rotatedY = scaledLocation.X * s + scaledLocation.Y * c;

    ImplPtr->WorldLocation.X = pLoc.X + rotatedX;
    ImplPtr->WorldLocation.Y = pLoc.Y + rotatedY;
  }

  ImplPtr->TransformDirty = false;
}
