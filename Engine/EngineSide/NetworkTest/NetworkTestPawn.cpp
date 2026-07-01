#include "NetworkTest/NetworkTestPawn.h"

#include <DxLib.h>
#include <MovementComponent.h>
#include <RectangleCollisionComponent.h>

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "NetworkManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
#include "NetworkTest/NetworkTestUI.h"
#include "SpriteComponent.h"
#include "World.h"

namespace {
enum : FNetworkRPCId {
  RPC_ServerTest = 1,
  RPC_MulticastTest = 2,
  RPC_ServerMove = 3,
  RPC_ComponentServerTest = 101,
  RPC_ComponentMulticastTest = 102
};
}

MNetworkTestRepComponent::MNetworkTestRepComponent() {
  bReplicates = true;
  SetNetComponentName("NetworkTestRepComponent");

  RegisterReplicatedProperty(
      &ReplicatedCounter, this, &MNetworkTestRepComponent::OnRepReplicatedCounter
  );
  RegisterRPC(
      RPC_ComponentServerTest,
      ENetRPCType::Server,
      this,
      &MNetworkTestRepComponent::Server_ComponentTest
  );
  RegisterRPC(
      RPC_ComponentMulticastTest,
      ENetRPCType::Multicast,
      this,
      &MNetworkTestRepComponent::Multicast_ComponentTest
  );
}

void MNetworkTestRepComponent::RequestTest(int PlayerId) {
  M_LOG(
      "Component RPC test input: actor={} component={} player={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId
  );
  InvokeRPC(
      RPC_ComponentServerTest, ENetRPCType::Server, ENetPacketReliability::Reliable, PlayerId
  );
}

void MNetworkTestRepComponent::OnUpdate(float DeltaTime) {
  if (OnRepFlashTimer > 0.0f) {
    OnRepFlashTimer -= DeltaTime;
    if (OnRepFlashTimer < 0.0f) {
      OnRepFlashTimer = 0.0f;
    }
  }

  if (RPCFlashTimer > 0.0f) {
    RPCFlashTimer -= DeltaTime;
    if (RPCFlashTimer < 0.0f) {
      RPCFlashTimer = 0.0f;
    }
  }

  if (RPCFlashTimer <= 0.0f && PendingOnRepFlashTimer > 0.0f) {
    OnRepFlashTimer = PendingOnRepFlashTimer;
    PendingOnRepFlashTimer = 0.0f;
  }
}

void MNetworkTestRepComponent::Server_ComponentTest(int PlayerId) {
  ++ReplicatedCounter;
  MarkReplicatedStateDirty();

  M_LOG(
      "Server_ComponentTest received: actor={} component={} player={} counter={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId,
      ReplicatedCounter
  );

  InvokeRPC(
      RPC_ComponentMulticastTest,
      ENetRPCType::Multicast,
      ENetPacketReliability::Reliable,
      PlayerId,
      ReplicatedCounter
  );
}

void MNetworkTestRepComponent::Multicast_ComponentTest(int PlayerId, int CounterValue) {
  M_LOG(
      "Multicast_ComponentTest received: actor={} component={} player={} counter={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      PlayerId,
      CounterValue
  );
  RPCFlashTimer = 0.8f;
}

void MNetworkTestRepComponent::OnRepReplicatedCounter(int OldValue) {
  M_LOG(
      "OnRep component counter: actor={} component={} old={} new={}",
      GetOwner() ? GetOwner()->NetworkId : 0,
      ComponentNetworkId,
      OldValue,
      ReplicatedCounter
  );
  if (RPCFlashTimer > 0.0f) {
    PendingOnRepFlashTimer = 0.75f;
  } else {
    OnRepFlashTimer = 0.75f;
  }
}

REGISTER_ACTOR(ANetworkTestPawn)

ANetworkTestPawn::ANetworkTestPawn() {
  bReplicates = true;
  NetworkTestUI = std::make_unique<FNetworkTestUI>(*this);

  RegisterRPC(RPC_ServerTest, ENetRPCType::Server, this, &ANetworkTestPawn::Server_TestRPC);
  RegisterRPC(
      RPC_MulticastTest, ENetRPCType::Multicast, this, &ANetworkTestPawn::Multicast_TestRPC
  );
  RegisterRPC(RPC_ServerMove, ENetRPCType::Server, this, &ANetworkTestPawn::Server_Move);

  auto sprite = std::make_unique<MSpriteComponent>(10, RenderSpace::World);
  BodySprite = sprite.get();
  BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
  BodySprite->SetRelativeLocation({-24.0f, -24.0f});
  AddComponent(std::move(sprite));

  auto col = std::make_unique<MRectangleCollisionComponent>(48.0f, 48.0f);
  col->SetParentComponent(GetRootComponent());
  col->SetStatic(false);
  AddComponent(std::move(col));

  auto movement = std::make_unique<MMovementComponent>();
  Movement = movement.get();
  AddComponent(std::move(movement));

  auto replicationTest = std::make_unique<MNetworkTestRepComponent>();
  ReplicationTest = replicationTest.get();
  AddComponent(std::move(replicationTest));
}

ANetworkTestPawn::~ANetworkTestPawn() = default;

void ANetworkTestPawn::BeginPlay() {
  APawn::BeginPlay();
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->InitializeStatus();
  }
}

void ANetworkTestPawn::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
}

void ANetworkTestPawn::OnUpdate(float DeltaTime) {
  if (FlashTimer > 0.0f) {
    FlashTimer -= DeltaTime;
    if (FlashTimer < 0.0f) {
      FlashTimer = 0.0f;
    }
  }

  if (BodySprite) {
    BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
  }
}

void ANetworkTestPawn::Draw() {
  APawn::Draw();
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->Draw();
  }
}

void ANetworkTestPawn::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  if (!PlayerInputComponent) {
    return;
  }

  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &ANetworkTestPawn::OnMove
  );
  PlayerInputComponent->BindAction(
      InputAction::Interact, ETriggerEvent::Started, this, &ANetworkTestPawn::OnInteract
  );
}

void ANetworkTestPawn::OnMove(const FInputActionValue& Value) {
  if (!bIsLocallyControlled) {
    return;
  }

  if (bHasAuthority) {
    ApplyMovementInput(Value.Axis2D);
  } else {
    InvokeRPC(RPC_ServerMove, ENetRPCType::Server, ENetPacketReliability::Unreliable, Value.Axis2D);
  }
}

void ANetworkTestPawn::Server_Move(const FVector2D& MoveInput) { ApplyMovementInput(MoveInput); }

void ANetworkTestPawn::ApplyMovementInput(const FVector2D& MoveInput) {
  if (Movement) {
    Movement->AddWorldForce(MoveInput * (-MoveSpeed / 60.0f) * 0.05f);
  }
}

void ANetworkTestPawn::OnInteract(const FInputActionValue& Value) {
  (void)Value;

  if (!bIsLocallyControlled) {
    return;
  }

  const int playerId = static_cast<int>(OwnerConnectionId);
  M_LOG("RPC test input: actor={} player={}", NetworkId, playerId);
  InvokeRPC(RPC_ServerTest, ENetRPCType::Server, ENetPacketReliability::Reliable, playerId);

  if (ReplicationTest) {
    ReplicationTest->RequestTest(playerId);
  }
}

void ANetworkTestPawn::Server_TestRPC(int PlayerId) {
  M_LOG("Server_TestRPC received: actor={} player={}", NetworkId, PlayerId);
  InvokeRPC(RPC_MulticastTest, ENetRPCType::Multicast, ENetPacketReliability::Reliable, PlayerId);
}

void ANetworkTestPawn::Multicast_TestRPC(int PlayerId) {
  M_LOG("Multicast_TestRPC received: actor={} player={}", NetworkId, PlayerId);
  FlashTimer = 0.5f;
}

int ANetworkTestPawn::GetDisplayColor() const {
  if (ReplicationTest && ReplicationTest->IsRPCFlashActive()) {
    return GetColor(255, 120, 255);
  }

  if (ReplicationTest && ReplicationTest->IsOnRepFlashActive()) {
    return GetColor(80, 170, 255);
  }

  if (FlashTimer > 0.0f) {
    return GetColor(255, 255, 80);
  }

  return (bIsLocallyControlled || (bHasAuthority && OwnerConnectionId == 0))
             ? GetColor(0, 220, 80)
             : GetColor(220, 60, 60);
}

void ANetworkTestPawn::BeginOverlap(AActor* OtherActor) {
  AActor::BeginOverlap(OtherActor);

  if (auto otherPawn = dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    M_LOG(
        "Player overlap detected! My NetworkId: {}, Other NetworkId: {}",
        NetworkId,
        otherPawn->NetworkId
    );
  }
}

void ANetworkTestPawn::SetStatusMessage(const std::string& Message) {
  if (bIsLocallyControlled && NetworkTestUI) {
    NetworkTestUI->SetStatusMessage(Message);
  }
}

bool ANetworkTestPawn::StartListenServer(uint16_t Port) {
  if (NetworkManager::GetInstance().StartServer(Port)) {
    GetWorld()->SetNetMode(ENetMode::ListenServer);
    bSessionStarted = true;
    return true;
  }

  return false;
}

bool ANetworkTestPawn::ConnectAsClient(const std::string& HostAddress, uint16_t Port) {
  if (NetworkManager::GetInstance().ConnectToServer(HostAddress, Port)) {
    GetWorld()->SetNetMode(ENetMode::Client);
    bSessionStarted = true;
    return true;
  }

  return false;
}
