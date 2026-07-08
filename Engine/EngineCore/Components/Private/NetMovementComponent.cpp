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
    CurrentInputAxis = FVector2D::ZeroVector;
    CurrentActions = EMoveAction::None;
    return;
  }

  if (OwnerActor->bIsLocallyControlled && !OwnerActor->bHasAuthority) {
    FMovePredictionData NewMove;
    NewMove.Sequence = ++CurrentSequence;
    NewMove.DeltaTime = DeltaTime;
    NewMove.InputAxis = CurrentInputAxis;
    NewMove.Actions = CurrentActions;

    InFlightMoves.push_back(NewMove);
    SimulateMovement(NewMove);
    InvokeRPC(
        RPC_ServerReceiveMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, NewMove
    );

    CurrentInputAxis = FVector2D::ZeroVector;
    CurrentActions = EMoveAction::None;
    return;
  }

  if (OwnerActor->bHasAuthority && OwnerActor->bIsLocallyControlled && ShouldSimulate()) {
    FMovePredictionData LocalMove;
    LocalMove.Sequence = ++CurrentSequence;
    LocalMove.DeltaTime = DeltaTime;
    LocalMove.InputAxis = CurrentInputAxis;
    LocalMove.Actions = CurrentActions;
    SimulateMovement(LocalMove);
  }

  CurrentInputAxis = FVector2D::ZeroVector;
  CurrentActions = EMoveAction::None;

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
  for (const FMovePredictionData& Move : ServerPendingMoves) {
    if (Move.Sequence <= ProcessedSequence) {
      continue;
    }

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
        GetVelocity()
    );
  }
}

void MNetMovementComponent::AddMovementInput(const FVector2D& Input) {
  CurrentInputAxis += Input;
}

void MNetMovementComponent::AddMovementAction(FMoveActionState Action) { CurrentActions |= Action; }

void MNetMovementComponent::SetMovementActions(FMoveActionState Actions) { CurrentActions = Actions; }

float MNetMovementComponent::GetMaxSpeedForActions(FMoveActionState Actions) const {
  (void)Actions;
  return MaxSpeed;
}

void MNetMovementComponent::SimulateMovement(const FMovePredictionData& Move) {
  if (Move.DeltaTime <= 0.0f) {
    return;
  }

  const FVector2D InputAxis = ClampInputAxis(Move.InputAxis);
  const float TargetMaxSpeed = GetMaxSpeedForActions(Move.Actions);

  FVector2D NewVelocity = GetVelocity();
  NewVelocity += InputAxis * (-Acceleration * Move.DeltaTime);
  NewVelocity = ClampVectorSize(NewVelocity, TargetMaxSpeed);
  NewVelocity *= std::pow(Friction, Move.DeltaTime * 60.0f);

  if (NewVelocity.SizeSquared() <= 0.001f) {
    NewVelocity = FVector2D::ZeroVector;
  }

  SetWorldForce(NewVelocity);
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
    uint32_t AckedSequence, FVector2D ServerLocation, FVector2D ServerVelocity
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
  SetWorldForce(ServerVelocity);
  std::erase_if(InFlightMoves, [AckedSequence](const FMovePredictionData& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (const FMovePredictionData& Move : InFlightMoves) {
    SimulateMovement(Move);
  }

  const FVector2D CorrectedLocation = OwnerActor->GetActorLocation();
  if ((CorrectedLocation - PredictedLocation).SizeSquared() <= CorrectionToleranceSquared) {
    OwnerActor->SetActorLocation(PredictedLocation);
  }
}

bool MNetMovementComponent::ShouldSimulate() const {
  return CurrentInputAxis.SizeSquared() > 0.0001f || CurrentActions != EMoveAction::None ||
         GetVelocitySizeSquared() > 0.001f;
}
