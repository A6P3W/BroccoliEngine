#include "SceneComponent.h"

#include <algorithm>
#include <cmath>

#include "Actor.h"
#include "Log.h"
#include "UMath.h"

const FAttachmentTransformRules FAttachmentTransformRules::KeepRelativeTransform(
    EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative
);

const FAttachmentTransformRules FAttachmentTransformRules::KeepWorldTransform(
    EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld
);

const FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetIncludingScale(
    EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget
);

MSceneComponent::MSceneComponent() = default;
MSceneComponent::~MSceneComponent() = default;

void MSceneComponent::OnUpdate(float DeltaTime) {}

void MSceneComponent::Draw() {}

void MSceneComponent::OnMessage(const std::string& message) {}

void MSceneComponent::OnComponentDestroy() {
  const auto children = ChildComponents;
  for (auto* child : children) {
    if (child != nullptr) {
      child->DestroyComponent();
    }
  }
  ChildComponents.clear();

  if (ParentComponent != nullptr) {
    std::erase(ParentComponent->ChildComponents, this);
    ParentComponent = nullptr;
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
    NewRelativeLocation = FVector2D::ZeroVector;
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

  if (ParentComponent != Parent) {
    if (ParentComponent != nullptr) {
      std::erase(ParentComponent->ChildComponents, this);
    }

    ParentComponent = Parent;

    if (ParentComponent != nullptr &&
        std::find(
            ParentComponent->ChildComponents.begin(), ParentComponent->ChildComponents.end(), this
        ) == ParentComponent->ChildComponents.end()) {
      ParentComponent->ChildComponents.push_back(this);
    }
  }

  RelativeLocation = NewRelativeLocation;
  RelativeRotation = NewRelativeRotation;
  RelativeScale = NewRelativeScale;

  MakeTransformDirty();
  return true;
}

bool MSceneComponent::SetWorldLocation(const FVector2D& NewWorldLocation) {
  if (!ParentComponent) {
    // 親がいない（RootComponentである）場合、
    // ワールド座標はそのまま相対座標（RelativeLocation）になる
    RelativeLocation = NewWorldLocation;
  } else {
    const FScale ParentWorldScale = ParentComponent->GetWorldScale();
    if (std::abs(ParentWorldScale.Scale) < 1e-6f) {
      return false;
    }

    // 親がいる場合、ワールド座標を親のローカル座標系に変換する必要がある
    FVector2D ParentWorldLoc = ParentComponent->GetWorldLocation();
    float ParentWorldRotRad = UMath::DegToRad(ParentComponent->GetWorldRotation().Rotation);

    // 1. 親との差分を取る
    float diffX = NewWorldLocation.X - ParentWorldLoc.X;
    float diffY = NewWorldLocation.Y - ParentWorldLoc.Y;

    // 2. 親の回転の「逆」をかけて、親から見た相対位置を出す
    // 逆回転行列: [ cos(-θ) -sin(-θ) ] = [  cosθ  sinθ ]
    //              [ sin(-θ)  cos(-θ) ]   [ -sinθ  cosθ ]
    float s = std::sin(ParentWorldRotRad);
    float c = std::cos(ParentWorldRotRad);

    RelativeLocation.X = diffX * c + diffY * s;
    RelativeLocation.Y = -diffX * s + diffY * c;
    RelativeLocation /= ParentWorldScale;
  }
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::SetRelativeLocation(const FVector2D& NewRelativeLocation) {
  RelativeLocation = NewRelativeLocation;
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
  if (!ParentComponent) {
    RelativeRotation = NewRotation;
  } else {
    FRotator parentWorldRot = ParentComponent->GetWorldRotation();
    RelativeRotation = NewRotation - parentWorldRot;
  }
  MakeTransformDirty();
  return true;
}

bool MSceneComponent::AddWorldRotation(FRotator DeltaRotation) {
  RelativeRotation += DeltaRotation;
  MakeTransformDirty();
  return true;
}

FVector2D MSceneComponent::GetWorldLocation() const {
  UpdateTransform();
  return WorldLocation;
}

FVector2D MSceneComponent::GetRelativeLocation() const { return RelativeLocation; }

FRotator MSceneComponent::GetWorldRotation() const {
  UpdateTransform();
  return WorldRotation;
}

FRotator MSceneComponent::GetRelativeRotation() const { return RelativeRotation; }

bool MSceneComponent::SetRelativeScale(FScale NewScale) {
  RelativeScale = NewScale;
  MakeTransformDirty();
  return true;
}

FScale MSceneComponent::GetRelativeScale() const { return RelativeScale; }

bool MSceneComponent::SetWorldScale(FScale NewScale) {
  if (!ParentComponent) {
    // 親がいなければワールドスケール = 相対スケール
    return SetRelativeScale(NewScale);
  }

  // 親がいる場合： 自分の相対 = 目標ワールド / 親のワールド
  FScale parentWorldScale = ParentComponent->GetWorldScale();
  if (std::abs(parentWorldScale.Scale) < 1e-6f) return false;

  return SetRelativeScale(NewScale / parentWorldScale);
}

FScale MSceneComponent::GetWorldScale() const {
  UpdateTransform();
  return WorldScale;
}

void MSceneComponent::SetVisibility(bool bNewVisibility) {
  bVisible = bNewVisibility;
  for (MSceneComponent* child : ChildComponents) {
    child->SetVisibility(bNewVisibility);
  }
}

void MSceneComponent::MakeTransformDirty() {
  if (bIsTransformDirty) {
    return;
  }
  bIsTransformDirty = true;
  bIsGridDirty = true;
  for (MSceneComponent* child : ChildComponents) {
    child->MakeTransformDirty();
  }
}

void MSceneComponent::UpdateTransform() const {
  if (!bIsTransformDirty) return;

  if (!ParentComponent) {
    // 親がいない場合は相対値がそのままワールド値
    WorldLocation = RelativeLocation;
    WorldRotation = RelativeRotation;
    WorldScale = RelativeScale;
  } else {
    // 親の最新トランスフォームを先に確定させる（再帰）
    FScale pScale = ParentComponent->GetWorldScale();
    FRotator pRot = ParentComponent->GetWorldRotation();
    FVector2D pLoc = ParentComponent->GetWorldLocation();

    // 1. スケールの計算
    WorldScale = RelativeScale * pScale;

    // 2. 回転の計算
    WorldRotation = pRot + RelativeRotation;

    // 3. 座標の計算（親の回転とスケールを考慮）
    float parentRad = UMath::DegToRad(pRot.Rotation);
    float s = std::sin(parentRad);
    float c = std::cos(parentRad);

    // 自分の相対座標に親のスケールを適用
    FVector2D scaledLocation = RelativeLocation * pScale;

    // 親の回転に合わせて回転させる
    float rotatedX = scaledLocation.X * c - scaledLocation.Y * s;
    float rotatedY = scaledLocation.X * s + scaledLocation.Y * c;

    WorldLocation.X = pLoc.X + rotatedX;
    WorldLocation.Y = pLoc.Y + rotatedY;
  }

  bIsTransformDirty = false;
}
