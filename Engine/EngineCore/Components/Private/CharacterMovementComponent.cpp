#include "CharacterMovementComponent.h"

#include <algorithm>
#include <cmath>

#include "Actor.h"
#include "CollisionSystem.h"
#include "World.h"

namespace {
enum : FNetworkRPCId {
  RPC_ServerReceiveMove = 1,
  RPC_ClientReceiveAck = 2,
};

constexpr float CorrectionTolerance = 4.0f;
constexpr float CorrectionToleranceSquared = CorrectionTolerance * CorrectionTolerance;

FVector2D ClampInputAxis(const FVector2D& InputAxis) {
  const float SizeSquared = InputAxis.SizeSquared();
  if (SizeSquared <= 1.0f || SizeSquared <= 0.0001f) {
    return InputAxis;
  }

  const float Size = std::sqrt(SizeSquared);
  return InputAxis / Size;
}

FVector2D ClampVectorSize(const FVector2D& Value, float MaxSize) {
  const float SizeSquared = Value.SizeSquared();
  const float MaxSizeSquared = MaxSize * MaxSize;
  if (MaxSize <= 0.0f || SizeSquared <= MaxSizeSquared || SizeSquared <= 0.0001f) {
    return Value;
  }

  const float Size = std::sqrt(SizeSquared);
  return Value * (MaxSize / Size);
}
}

MCharacterMovementComponent::MCharacterMovementComponent() {
  bReplicates = true;
  SetNetComponentName("CharacterMovementComponent");
  RegisterReplicatedProperty(&Friction);
  RegisterReplicatedProperty(&MaxSpeed);
  RegisterReplicatedProperty(&Acceleration);
  RegisterReplicatedProperty(&DashSpeedMultiplier);
}

void MCharacterMovementComponent::RegisterComponent() {
  MMovementComponent::RegisterComponent();

  RegisterRPC(
      RPC_ServerReceiveMove,
      ENetRPCType::Server,
      this,
      &MCharacterMovementComponent::Server_ReceiveMove
  );
  RegisterRPC(
      RPC_ClientReceiveAck,
      ENetRPCType::Client,
      this,
      &MCharacterMovementComponent::Client_ReceiveAck
  );
}

void MCharacterMovementComponent::OnUpdate(float DeltaTime) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || DeltaTime <= 0.0f) {
    CurrentInputAxis = FVector2D::ZeroVector;
    CurrentInputFlags = 0;
    return;
  }

  if (OwnerActor->bIsLocallyControlled && !OwnerActor->bHasAuthority) {
    FSavedMove NewMove;
    NewMove.Sequence = ++CurrentSequence;
    NewMove.DeltaTime = DeltaTime;
    NewMove.InputAxis = CurrentInputAxis;
    NewMove.SavedFlags = CurrentInputFlags;

    SavedMoves.push_back(NewMove);
    PerformMovement(NewMove);
    InvokeRPC(
        RPC_ServerReceiveMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, NewMove
    );

    CurrentInputAxis = FVector2D::ZeroVector;
    CurrentInputFlags = 0;
    return;
  }

  if (OwnerActor->bHasAuthority && OwnerActor->bIsLocallyControlled && HasPendingSimulation()) {
    FSavedMove LocalMove;
    LocalMove.Sequence = ++CurrentSequence;
    LocalMove.DeltaTime = DeltaTime;
    LocalMove.InputAxis = CurrentInputAxis;
    LocalMove.SavedFlags = CurrentInputFlags;
    PerformMovement(LocalMove);
  }

  CurrentInputAxis = FVector2D::ZeroVector;
  CurrentInputFlags = 0;

  if (!OwnerActor->bHasAuthority) {
    return;
  }

  std::stable_sort(
      ServerPendingMoves.begin(),
      ServerPendingMoves.end(),
      [](const FSavedMove& Lhs, const FSavedMove& Rhs) {
        return Lhs.Sequence < Rhs.Sequence;
      }
  );

  bool bProcessedMove = false;
  uint32_t ProcessedSequence = LastAckedSequence;
  for (const FSavedMove& Move : ServerPendingMoves) {
    if (Move.Sequence <= ProcessedSequence) {
      continue;
    }

    PerformMovement(Move);
    ProcessedSequence = Move.Sequence;
    bProcessedMove = true;
  }
  ServerPendingMoves.clear();

  if (bProcessedMove) {
    LastAckedSequence = ProcessedSequence;
    InvokeRPC(
        RPC_ClientReceiveAck,
        ENetRPCType::Client,
        ENetPacketReliability::Unreliable,
        LastAckedSequence,
        OwnerActor->GetActorLocation(),
        GetVelocity()
    );
  }
}

void MCharacterMovementComponent::AddInputVector(const FVector2D& Input) {
  CurrentInputAxis += Input;
}

void MCharacterMovementComponent::AddInputFlag(uint8_t Flag) { CurrentInputFlags |= Flag; }

void MCharacterMovementComponent::PerformMovement(const FSavedMove& Move) {
  if (Move.DeltaTime <= 0.0f) {
    return;
  }

  const FVector2D InputAxis = ClampInputAxis(Move.InputAxis);
  const bool bWantsDash = (Move.SavedFlags & FLAG_Dash) != 0;
  const float SpeedMultiplier = bWantsDash ? DashSpeedMultiplier : 1.0f;
  const float TargetMaxSpeed = MaxSpeed * SpeedMultiplier;

  FVector2D NewVelocity = GetVelocity();
  NewVelocity += InputAxis * (-Acceleration * SpeedMultiplier * Move.DeltaTime);
  NewVelocity = ClampVectorSize(NewVelocity, TargetMaxSpeed);
  NewVelocity *= std::pow(Friction, Move.DeltaTime * 60.0f);

  if (NewVelocity.SizeSquared() <= 0.001f) {
    NewVelocity = FVector2D::ZeroVector;
  }

  SetWorldForce(NewVelocity);
  MoveWithCollision(NewVelocity * Move.DeltaTime);
}

void MCharacterMovementComponent::MoveWithCollision(const FVector2D& DesiredOffset) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || DesiredOffset.SizeSquared() <= 0.0001f) {
    return;
  }

  OwnerActor->AddActorWorldOffset(DesiredOffset);
}

void MCharacterMovementComponent::Server_ReceiveMove(FSavedMove Move) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->bHasAuthority) {
    return;
  }

  ServerPendingMoves.push_back(Move);
}

void MCharacterMovementComponent::Client_ReceiveAck(
    uint32_t AckedSequence, FVector2D ServerLocation, FVector2D ServerVelocity
) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || OwnerActor->bHasAuthority) {
    return;
  }

  if (AckedSequence <= LastAckedSequence) {
    return;
  }

  LastAckedSequence = AckedSequence;

  const FVector2D PredictedLocation = OwnerActor->GetActorLocation();

  OwnerActor->SetActorLocation(ServerLocation);
  SetWorldForce(ServerVelocity);
  std::erase_if(SavedMoves, [AckedSequence](const FSavedMove& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (const FSavedMove& Move : SavedMoves) {
    PerformMovement(Move);
  }

  const FVector2D CorrectedLocation = OwnerActor->GetActorLocation();
  if ((CorrectedLocation - PredictedLocation).SizeSquared() <= CorrectionToleranceSquared) {
    OwnerActor->SetActorLocation(PredictedLocation);
  }
}

bool MCharacterMovementComponent::HasPendingSimulation() const {
  return CurrentInputAxis.SizeSquared() > 0.0001f || CurrentInputFlags != 0 ||
         GetVelocitySizeSquared() > 0.001f;
}
