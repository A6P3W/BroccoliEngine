#include "NetMovementComponent.h"

#include <algorithm>
#include <vector>

#include "Actor.h"
#include "World.h"

namespace {
enum : FNetworkRPCId {
  RPC_ServerReceiveMove = 1,
  RPC_ClientReceiveAck = 2,
};
}

struct MNetMovementComponent::Impl {
  std::vector<FMovePredictionData> InFlightMoves;
  std::vector<FMovePredictionData> ServerPendingMoves;
  uint32_t CurrentSequence = 0;
  uint32_t LastConfirmedSequence = 0;
  FVector2D FrameForce = FVector2D::ZeroVector();
  FVector2D FrameVelocityOverride = FVector2D::ZeroVector();
  FRotator FrameVelocityRotation = FRotator(0.0f);
  bool bFrameVelocityOverride = false;
};

MNetMovementComponent::MNetMovementComponent() : ImplPtr(new Impl()) {
  bReplicates = true;
  SetNetComponentName("NetMovementComponent");
  RegisterReplicatedProperty(&Friction);
}

MNetMovementComponent::~MNetMovementComponent() { delete ImplPtr; }

void MNetMovementComponent::OnRegister() {
  MMovementComponent::OnRegister();

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
    ClearFrameMovementData();
    return;
  }

  if (OwnerActor->bIsLocallyControlled && !OwnerActor->bHasAuthority) {
    if (ShouldSimulate()) {
      FMovePredictionData NewMove;
      NewMove.Sequence = ++ImplPtr->CurrentSequence;
      NewMove.DeltaTime = DeltaTime;
      NewMove.Force = ImplPtr->FrameForce;
      NewMove.StartVelocity = GetVelocity();
      NewMove.VelocityOverride = ImplPtr->FrameVelocityOverride;
      NewMove.bUseVelocityOverride = ImplPtr->bFrameVelocityOverride;
      NewMove.StartRotation = OwnerActor->GetActorRotation();
      NewMove.VelocityRotation = ImplPtr->FrameVelocityRotation;

      ImplPtr->InFlightMoves.push_back(NewMove);
      SimulateMovement(NewMove);
      InvokeRPC(
          RPC_ServerReceiveMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, NewMove
      );
    }

    ClearFrameMovementData();
    return;
  }

  const bool bHasAuthorityFrameInput =
      ImplPtr->FrameForce.SizeSquared() > 0.0001f ||
      ImplPtr->FrameVelocityRotation.Rotation != 0.0f || ImplPtr->bFrameVelocityOverride;

  if (OwnerActor->bHasAuthority &&
      (bHasAuthorityFrameInput || ImplPtr->ServerPendingMoves.empty()) && ShouldSimulate()) {
    FMovePredictionData AuthorityMove;
    AuthorityMove.DeltaTime = DeltaTime;
    AuthorityMove.Force = ImplPtr->FrameForce;
    AuthorityMove.StartVelocity = GetVelocity();
    AuthorityMove.VelocityOverride = ImplPtr->FrameVelocityOverride;
    AuthorityMove.bUseVelocityOverride = ImplPtr->bFrameVelocityOverride;
    AuthorityMove.StartRotation = OwnerActor->GetActorRotation();
    AuthorityMove.VelocityRotation = ImplPtr->FrameVelocityRotation;
    SimulateMovement(AuthorityMove);
  }

  ClearFrameMovementData();

  if (!OwnerActor->bHasAuthority) return;

  std::stable_sort(
      ImplPtr->ServerPendingMoves.begin(),
      ImplPtr->ServerPendingMoves.end(),
      [](const FMovePredictionData& Lhs, const FMovePredictionData& Rhs) {
        return Lhs.Sequence < Rhs.Sequence;
      }
  );

  bool bProcessedMove = false;
  uint32_t ProcessedSequence = ImplPtr->LastConfirmedSequence;
  for (FMovePredictionData& Move : ImplPtr->ServerPendingMoves) {
    if (Move.Sequence <= ProcessedSequence) continue;

    if (!Move.bUseVelocityOverride) {
      Move.StartVelocity = GetVelocity();
    }
    SimulateMovement(Move);
    ProcessedSequence = Move.Sequence;
    bProcessedMove = true;
  }
  ImplPtr->ServerPendingMoves.clear();

  if (bProcessedMove) {
    ImplPtr->LastConfirmedSequence = ProcessedSequence;
    InvokeRPC(
        RPC_ClientReceiveAck,
        ENetRPCType::Client,
        ENetPacketReliability::Unreliable,
        ImplPtr->LastConfirmedSequence,
        OwnerActor->GetActorLocation(),
        OwnerActor->GetActorRotation(),
        GetVelocity()
    );
  }
}

void MNetMovementComponent::AddWorldForce(const FVector2D& Force) {
  ImplPtr->FrameForce = ImplPtr->FrameForce + Force;
}

void MNetMovementComponent::AddLocalForce(const FVector2D& Force) {
  AActor* OwnerActor = GetOwner();
  AddWorldForce(OwnerActor ? Force.RotateVector(OwnerActor->GetActorRotation()) : Force);
}

void MNetMovementComponent::SetWorldForce(const FVector2D& Force) {
  ImplPtr->FrameForce = Force;
}

void MNetMovementComponent::SetLocalForce(const FVector2D& Force) {
  AActor* OwnerActor = GetOwner();
  SetWorldForce(OwnerActor ? Force.RotateVector(OwnerActor->GetActorRotation()) : Force);
}

void MNetMovementComponent::SetWorldVelocity(const FVector2D& NewVelocity) {
  MMovementComponent::SetWorldVelocity(NewVelocity);
  ImplPtr->FrameVelocityOverride = NewVelocity;
  ImplPtr->bFrameVelocityOverride = true;
}

void MNetMovementComponent::SetLocalVelocity(const FVector2D& NewVelocity) {
  AActor* OwnerActor = GetOwner();
  SetWorldVelocity(
      OwnerActor ? NewVelocity.RotateVector(OwnerActor->GetActorRotation()) : NewVelocity
  );
}

void MNetMovementComponent::AddWorldVelocity(const FVector2D& DeltaVelocity) {
  SetWorldVelocity(GetVelocity() + DeltaVelocity);
}

void MNetMovementComponent::AddLocalVelocity(const FVector2D& DeltaVelocity) {
  AActor* OwnerActor = GetOwner();
  AddWorldVelocity(
      OwnerActor ? DeltaVelocity.RotateVector(OwnerActor->GetActorRotation()) : DeltaVelocity
  );
}

void MNetMovementComponent::AddVelocityRotation(const FRotator& Rotation) {
  ImplPtr->FrameVelocityRotation = ImplPtr->FrameVelocityRotation + Rotation;
}

void MNetMovementComponent::SetVelocityRotation(const FRotator& Rotation) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) return;
  ImplPtr->FrameVelocityRotation = Rotation - OwnerActor->GetActorRotation();
}

void MNetMovementComponent::SimulateMovement(const FMovePredictionData& Move) {
  FMovementStepData Step;
  Step.DeltaTime = Move.DeltaTime;
  Step.Force = Move.Force;
  Step.StartVelocity =
      Move.bUseVelocityOverride ? Move.VelocityOverride : Move.StartVelocity;
  Step.StartRotation = Move.StartRotation;
  Step.VelocityRotation = Move.VelocityRotation;
  SimulateMovementStep(Step);
}

void MNetMovementComponent::Server_UploadMove(FMovePredictionData Move) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->bHasAuthority) return;
  ImplPtr->ServerPendingMoves.push_back(Move);
}

void MNetMovementComponent::Client_ResolveMove(
    uint32_t AckedSequence,
    FVector2D ServerLocation,
    FRotator ServerRotation,
    FVector2D ServerVelocity
) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || OwnerActor->bHasAuthority ||
      AckedSequence <= ImplPtr->LastConfirmedSequence) {
    return;
  }

  ImplPtr->LastConfirmedSequence = AckedSequence;
  OwnerActor->SetActorLocation(ServerLocation);
  OwnerActor->SetActorRotation(ServerRotation);
  MMovementComponent::SetWorldVelocity(ServerVelocity);

  std::erase_if(ImplPtr->InFlightMoves, [AckedSequence](const FMovePredictionData& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (FMovePredictionData& Move : ImplPtr->InFlightMoves) {
    if (!Move.bUseVelocityOverride) {
      Move.StartVelocity = GetVelocity();
    }
    SimulateMovement(Move);
  }
}

bool MNetMovementComponent::ShouldSimulate() const {
  return ImplPtr->FrameForce.SizeSquared() > 0.0001f ||
         ImplPtr->FrameVelocityRotation.Rotation != 0.0f ||
         ImplPtr->bFrameVelocityOverride || GetVelocitySizeSquared() > 0.001f;
}

void MNetMovementComponent::ClearFrameMovementData() {
  ImplPtr->FrameForce = FVector2D::ZeroVector();
  ImplPtr->FrameVelocityRotation = FRotator(0.0f);
  ImplPtr->FrameVelocityOverride = FVector2D::ZeroVector();
  ImplPtr->bFrameVelocityOverride = false;
  ClearPendingMovement();
}
