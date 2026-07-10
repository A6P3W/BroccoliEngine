#include "NetMovementComponent.h"

#include <algorithm>
#include <cmath>

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
    ClearFrameMovementData();
    return;
  }

  if (OwnerActor->bIsLocallyControlled && !OwnerActor->bHasAuthority) {
    if (ShouldSimulate()) {
      FMovePredictionData NewMove;
      NewMove.Sequence = ++CurrentSequence;
      NewMove.DeltaTime = DeltaTime;
      NewMove.CurrentForce = CurrentForce;
      NewMove.Velocity = bFrameVelocityOverride ? FrameVelocityOverride : GetVelocity();
      NewMove.bUseVelocityOverride = bFrameVelocityOverride;
      NewMove.StartRotation = OwnerActor->GetActorRotation();
      NewMove.VelocityRotation = VelocityRotation;

      InFlightMoves.push_back(NewMove);
      SimulateMovement(NewMove);
      InvokeRPC(
          RPC_ServerReceiveMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, NewMove
      );
    }

    ClearFrameMovementData();
    return;
  }

  const bool bHasAuthorityFrameInput =
      CurrentForce.SizeSquared() > 0.0001f || VelocityRotation.Rotation != 0.0f ||
      bFrameVelocityOverride;
  if (OwnerActor->bHasAuthority && (bHasAuthorityFrameInput || ServerPendingMoves.empty()) &&
      ShouldSimulate()) {
    FMovePredictionData LocalMove;
    LocalMove.Sequence = ++CurrentSequence;
    LocalMove.DeltaTime = DeltaTime;
    LocalMove.CurrentForce = CurrentForce;
    LocalMove.Velocity = bFrameVelocityOverride ? FrameVelocityOverride : GetVelocity();
    LocalMove.bUseVelocityOverride = bFrameVelocityOverride;
    LocalMove.StartRotation = OwnerActor->GetActorRotation();
    LocalMove.VelocityRotation = VelocityRotation;
    SimulateMovement(LocalMove);
  }

  ClearFrameMovementData();

  if (!OwnerActor->bHasAuthority) {
    return;
  }

  if (bHasAuthorityFrameInput) {
    ServerPendingMoves.clear();
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

    if (!Move.bUseVelocityOverride) {
      Move.Velocity = GetVelocity();
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
        OwnerActor->GetActorRotation(),
        GetVelocity()
    );
  }
}

void MNetMovementComponent::AddMovementInput(const FVector2D& Input) {
  AddWorldForce(Input * -(MaxSpeed * (1.0f - Friction)));
}

void MNetMovementComponent::AddWorldForce(const FVector2D& Force) {
  CurrentForce = CurrentForce + Force;
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
  FrameVelocityOverride = Force;
  bFrameVelocityOverride = true;
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
  VelocityRotation = VelocityRotation + Rotation;
}

void MNetMovementComponent::SetVelocityRotation(const FRotator& Rotation) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor) {
    return;
  }

  const FRotator CurrentRotation = OwnerActor->GetActorRotation();
  VelocityRotation = Rotation - CurrentRotation;
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
    NewVelocity = FVector2D::ZeroVector;
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

  OwnerActor->SetActorLocation(ServerLocation);
  OwnerActor->SetActorRotation(ServerRotation);
  MMovementComponent::SetWorldForce(ServerVelocity);
  std::erase_if(InFlightMoves, [AckedSequence](const FMovePredictionData& Move) {
    return Move.Sequence <= AckedSequence;
  });

  for (FMovePredictionData& Move : InFlightMoves) {
    SimulateMovement(Move);
  }
}

bool MNetMovementComponent::ShouldSimulate() const {
  return CurrentForce.SizeSquared() > 0.0001f || VelocityRotation.Rotation != 0.0f ||
         bFrameVelocityOverride || GetVelocitySizeSquared() > 0.001f;
}

void MNetMovementComponent::ClearFrameMovementData() {
  CurrentForce = FVector2D::ZeroVector;
  VelocityRotation = FRotator(0.0f);
  FrameVelocityOverride = FVector2D::ZeroVector;
  bFrameVelocityOverride = false;
}
