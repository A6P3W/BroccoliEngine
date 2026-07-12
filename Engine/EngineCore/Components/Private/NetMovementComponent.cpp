#include "NetMovementComponent.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Actor.h"
#include "CollisionSystem.h"
#include "World.h"
#include "EngineDefine.h"

namespace {
enum : FNetworkRPCId {
  RPC_ServerReceiveMove = 1,
  RPC_ClientReceiveAck = 2,
};

}

struct MNetMovementComponent::Impl {
  FVector2D CurrentForce = FVector2D::ZeroVector();
  FRotator VelocityRotation = FRotator(0.0f);
  FVector2D FrameVelocityOverride = FVector2D::ZeroVector();
  bool bFrameVelocityOverride = false;
  std::vector<FMovePredictionData> InFlightMoves;
  std::vector<FMovePredictionData> ServerPendingMoves;
  uint32_t CurrentSequence = 0;
  uint32_t LastConfirmedSequence = 0;
  float MaxSpeed = 320.0f;
  float Acceleration = 2800.0f;
};

MNetMovementComponent::MNetMovementComponent() : ImplPtr(new Impl()) {
  bReplicates = true;
  SetNetComponentName("NetMovementComponent");
  RegisterReplicatedProperty(&Friction);
  RegisterReplicatedProperty(&ImplPtr->MaxSpeed);
  RegisterReplicatedProperty(&ImplPtr->Acceleration);
}

MNetMovementComponent::~MNetMovementComponent() { delete ImplPtr; }

void MNetMovementComponent::SetMaxSpeed(float NewMaxSpeed) { ImplPtr->MaxSpeed = NewMaxSpeed; }
float MNetMovementComponent::GetMaxSpeed() const { return ImplPtr->MaxSpeed; }
void MNetMovementComponent::SetAcceleration(float NewAcceleration) { ImplPtr->Acceleration = NewAcceleration; }
float MNetMovementComponent::GetAcceleration() const { return ImplPtr->Acceleration; }

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
      NewMove.CurrentForce = ImplPtr->CurrentForce;
      NewMove.Velocity = ImplPtr->bFrameVelocityOverride ? ImplPtr->FrameVelocityOverride : GetVelocity();
      NewMove.bUseVelocityOverride = ImplPtr->bFrameVelocityOverride;
      NewMove.StartRotation = OwnerActor->GetActorRotation();
      NewMove.VelocityRotation = ImplPtr->VelocityRotation;

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
      ImplPtr->CurrentForce.SizeSquared() > 0.0001f || ImplPtr->VelocityRotation.Rotation != 0.0f ||
      ImplPtr->bFrameVelocityOverride;
  if (OwnerActor->bHasAuthority && (bHasAuthorityFrameInput || ImplPtr->ServerPendingMoves.empty()) &&
      ShouldSimulate()) {
    FMovePredictionData LocalMove;
    LocalMove.Sequence = ++ImplPtr->CurrentSequence;
    LocalMove.DeltaTime = DeltaTime;
    LocalMove.CurrentForce = ImplPtr->CurrentForce;
    LocalMove.Velocity = ImplPtr->bFrameVelocityOverride ? ImplPtr->FrameVelocityOverride : GetVelocity();
    LocalMove.bUseVelocityOverride = ImplPtr->bFrameVelocityOverride;
    LocalMove.StartRotation = OwnerActor->GetActorRotation();
    LocalMove.VelocityRotation = ImplPtr->VelocityRotation;
    SimulateMovement(LocalMove);
  }

  ClearFrameMovementData();

  if (!OwnerActor->bHasAuthority) {
    return;
  }

  if (bHasAuthorityFrameInput) {
    ImplPtr->ServerPendingMoves.clear();
    return;
  }

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
    if (Move.Sequence <= ProcessedSequence) {
      continue;
    }

    if (!Move.bUseVelocityOverride) {
      Move.Velocity = GetVelocity();
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

void MNetMovementComponent::AddMovementInput(const FVector2D& Input) {
  AddWorldForce(Input * -(ImplPtr->MaxSpeed * (1.0f - Friction)));
}

void MNetMovementComponent::AddWorldForce(const FVector2D& Force) {
  ImplPtr->CurrentForce = ImplPtr->CurrentForce + Force;
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
  MMovementComponent::SetWorldForce(Force);
  ImplPtr->FrameVelocityOverride = Force;
  ImplPtr->bFrameVelocityOverride = true;
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
  ImplPtr->VelocityRotation = ImplPtr->VelocityRotation + Rotation;
}

void MNetMovementComponent::SetVelocityRotation(const FRotator& Rotation) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) {
    return;
  }

  const FRotator CurrentRotation = OwnerActor->GetActorRotation();
  ImplPtr->VelocityRotation = Rotation - CurrentRotation;
}

void MNetMovementComponent::SimulateMovement(const FMovePredictionData& Move) {
  if (Move.DeltaTime <= 0.0f) {
    return;
  }

  AActor* OwnerActor = GetOwner();
  if (OwnerActor) {
    OwnerActor->SetActorRotation(Move.StartRotation);
  }

  const float FrameScale = Move.DeltaTime * ReferenceFrameRate;
  FVector2D NewVelocity = Move.Velocity;
  NewVelocity += Move.CurrentForce * FrameScale;
  NewVelocity = NewVelocity.RotateVector(FRotator(Move.VelocityRotation.Rotation * FrameScale));
  NewVelocity *= std::pow(Friction, Move.DeltaTime * ReferenceFrameRate);

  if (NewVelocity.SizeSquared() <= 0.001f) {
    NewVelocity = FVector2D::ZeroVector();
  }

  MMovementComponent::SetWorldForce(NewVelocity);
  ApplyMovementData(NewVelocity * FrameScale);
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

  ImplPtr->ServerPendingMoves.push_back(Move);
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

  if (AckedSequence <= ImplPtr->LastConfirmedSequence) {
    return;
  }

  ImplPtr->LastConfirmedSequence = AckedSequence;

  OwnerActor->SetActorLocation(ServerLocation);
  OwnerActor->SetActorRotation(ServerRotation);
  MMovementComponent::SetWorldForce(ServerVelocity);
  std::erase_if(ImplPtr->InFlightMoves, [AckedSequence](const FMovePredictionData& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (FMovePredictionData& Move : ImplPtr->InFlightMoves) {
    SimulateMovement(Move);
  }
}

bool MNetMovementComponent::ShouldSimulate() const {
  return ImplPtr->CurrentForce.SizeSquared() > 0.0001f || ImplPtr->VelocityRotation.Rotation != 0.0f ||
         ImplPtr->bFrameVelocityOverride || GetVelocitySizeSquared() > 0.001f;
}

void MNetMovementComponent::ClearFrameMovementData() {
  ImplPtr->CurrentForce = FVector2D::ZeroVector();
  ImplPtr->VelocityRotation = FRotator(0.0f);
  ImplPtr->FrameVelocityOverride = FVector2D::ZeroVector();
  ImplPtr->bFrameVelocityOverride = false;
}
