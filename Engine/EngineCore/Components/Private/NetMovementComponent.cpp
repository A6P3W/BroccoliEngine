#include "NetMovementComponent.h"

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

MNetMovementComponent::MNetMovementComponent() {
  bReplicates = true;
  SetNetComponentName("NetMovementComponent");
  RegisterReplicatedProperty(&Friction);
  RegisterReplicatedProperty(&MaxSpeed);
  RegisterReplicatedProperty(&Acceleration);
}

void MNetMovementComponent::RegisterComponent() {
  MMovementComponent::RegisterComponent();

  RegisterRPC(
      RPC_ServerReceiveMove,
      ENetRPCType::Server,
      this,
      &MNetMovementComponent::Server_UploadMove
  );
  RegisterRPC(
      RPC_ClientReceiveAck,
      ENetRPCType::Client,
      this,
      &MNetMovementComponent::Client_ResolveMove
  );
}

void MNetMovementComponent::OnUpdate(float DeltaTime) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || DeltaTime <= 0.0f) {
    CurrentDesiredVelocity = FVector2D::ZeroVector;
    return;
  }

  if (OwnerActor->bIsLocallyControlled && !OwnerActor->bHasAuthority) {
    FMovePredictionData NewMove;
    NewMove.Sequence = ++CurrentSequence;
    NewMove.DeltaTime = DeltaTime;
    NewMove.DesiredVelocity = CurrentDesiredVelocity;
    NewMove.StartRotation = OwnerActor->GetActorRotation();

    InFlightMoves.push_back(NewMove);
    SimulateMovement(NewMove);
    InvokeRPC(
        RPC_ServerReceiveMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, NewMove
    );

    CurrentDesiredVelocity = FVector2D::ZeroVector;
    return;
  }

  if (OwnerActor->bHasAuthority && OwnerActor->bIsLocallyControlled && ShouldSimulate()) {
    FMovePredictionData LocalMove;
    LocalMove.Sequence = ++CurrentSequence;
    LocalMove.DeltaTime = DeltaTime;
    LocalMove.DesiredVelocity = CurrentDesiredVelocity;
    LocalMove.StartRotation = OwnerActor->GetActorRotation();
    SimulateMovement(LocalMove);
  }

  CurrentDesiredVelocity = FVector2D::ZeroVector;

  if (!OwnerActor->bHasAuthority) {
    return;
  }

  std::stable_sort(
      ServerPendingMoves.begin(),
      ServerPendingMoves.end(),
      [](const FMovePredictionData& Lhs, const FMovePredictionData& Rhs) {
        return Lhs.Sequence < Rhs.Sequence;
      }
  );

  bool bProcessedMove = false;
  uint32_t ProcessedSequence = LastConfirmedSequence;
  for (FMovePredictionData& Move : ServerPendingMoves) {
    if (Move.Sequence <= ProcessedSequence) {
      continue;
    }

    Move.StartRotation = OwnerActor->GetActorRotation();
    SimulateMovement(Move);
    ProcessedSequence = Move.Sequence;
    bProcessedMove = true;
  }
  ServerPendingMoves.clear();

  if (bProcessedMove) {
    LastConfirmedSequence = ProcessedSequence;
    InvokeRPC(
        RPC_ClientReceiveAck,
        ENetRPCType::Client,
        ENetPacketReliability::Unreliable,
        LastConfirmedSequence,
        OwnerActor->GetActorLocation(),
        OwnerActor->GetActorRotation(),
        GetVelocity()
    );
  }
}

void MNetMovementComponent::AddMovementInput(const FVector2D& Input) {
  AddWorldForce(Input * -MaxSpeed);
}

void MNetMovementComponent::AddWorldForce(const FVector2D& Force) {
  CurrentDesiredVelocity = CurrentDesiredVelocity + Force;
}

void MNetMovementComponent::AddLocalForce(const FVector2D& Force) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) {
    AddWorldForce(Force);
    return;
  }

  AddWorldForce(Force.RotateVector(OwnerActor->GetActorRotation()));
}

void MNetMovementComponent::SetWorldForce(const FVector2D& Force) {
  CurrentDesiredVelocity = Force;
}

void MNetMovementComponent::SetLocalForce(const FVector2D& Force) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) {
    SetWorldForce(Force);
    return;
  }

  SetWorldForce(Force.RotateVector(OwnerActor->GetActorRotation()));
}

void MNetMovementComponent::AddVelocityRotation(const FRotator& Rotation) {
  CurrentDesiredVelocity = CurrentDesiredVelocity.RotateVector(Rotation);
}

void MNetMovementComponent::SetVelocityRotation(const FRotator& Rotation) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) {
    return;
  }

  const FRotator CurrentRotation = OwnerActor->GetActorRotation();
  CurrentDesiredVelocity = CurrentDesiredVelocity.RotateVector(Rotation - CurrentRotation);
}

float MNetMovementComponent::GetMaxSpeedForDesiredVelocity(
    const FVector2D& DesiredVelocity
) const {
  (void)DesiredVelocity;
  return MaxSpeed;
}

void MNetMovementComponent::SimulateMovement(const FMovePredictionData& Move) {
  if (Move.DeltaTime <= 0.0f) {
    return;
  }

  const FVector2D DesiredVelocity = ClampVectorSize(
      Move.DesiredVelocity, GetMaxSpeedForDesiredVelocity(Move.DesiredVelocity)
  );
  const FVector2D VelocityDelta = DesiredVelocity - GetVelocity();
  const FVector2D AccelerationDelta = ClampVectorSize(VelocityDelta, Acceleration * Move.DeltaTime);

  FVector2D NewVelocity = GetVelocity() + AccelerationDelta;
  NewVelocity *= std::pow(Friction, Move.DeltaTime * 60.0f);

  if (NewVelocity.SizeSquared() <= 0.001f) {
    NewVelocity = FVector2D::ZeroVector;
  }

  MMovementComponent::SetWorldForce(NewVelocity);
  ApplyMovementData(NewVelocity * Move.DeltaTime);
}

void MNetMovementComponent::ApplyMovementData(const FVector2D& DesiredOffset) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || DesiredOffset.SizeSquared() <= 0.0001f) {
    return;
  }

  OwnerActor->AddActorWorldOffset(DesiredOffset);
}

void MNetMovementComponent::Server_UploadMove(FMovePredictionData Move) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->bHasAuthority) {
    return;
  }

  ServerPendingMoves.push_back(Move);
}

void MNetMovementComponent::Client_ResolveMove(
    uint32_t AckedSequence,
    FVector2D ServerLocation,
    FRotator ServerRotation,
    FVector2D ServerVelocity
) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || OwnerActor->bHasAuthority) {
    return;
  }

  if (AckedSequence <= LastConfirmedSequence) {
    return;
  }

  LastConfirmedSequence = AckedSequence;

  const FVector2D PredictedLocation = OwnerActor->GetActorLocation();

  OwnerActor->SetActorLocation(ServerLocation);
  OwnerActor->SetActorRotation(ServerRotation);
  MMovementComponent::SetWorldForce(ServerVelocity);
  std::erase_if(InFlightMoves, [AckedSequence](const FMovePredictionData& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (FMovePredictionData& Move : InFlightMoves) {
    Move.StartRotation = OwnerActor->GetActorRotation();
    SimulateMovement(Move);
  }

  const FVector2D CorrectedLocation = OwnerActor->GetActorLocation();
  if ((CorrectedLocation - PredictedLocation).SizeSquared() <= CorrectionToleranceSquared) {
    OwnerActor->SetActorLocation(PredictedLocation);
  }
}

bool MNetMovementComponent::ShouldSimulate() const {
  return CurrentDesiredVelocity.SizeSquared() > 0.0001f || GetVelocitySizeSquared() > 0.001f;
}
