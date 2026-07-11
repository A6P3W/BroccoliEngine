#include "NetworkTestPawn.h"

#include <DxLib.h>
#include <NetMovementComponent.h>
#include <RectangleCollisionComponent.h>

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "NetworkManager.h"
#include "NetworkTestGameMode.h"
#include "NetworkTestUI.h"
#include "SpriteComponent.h"
#include "World.h"

namespace {
enum : FNetworkRPCId {
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

  BodySprite = NewObject<MSpriteComponent>(this);
  BodySprite->SetRenderSettings(10, RenderSpace::World);
  BodySprite->SubmitBox(48.0f, 48.0f, GetDisplayColor(), true);
  BodySprite->SetRelativeLocation({-24.0f, -24.0f});
  BodySprite->RegisterComponent();

  auto* CollisionComponent = NewObject<MRectangleCollisionComponent>(this);
  CollisionComponent->SetSize(48.0f, 48.0f);
  CollisionComponent->AttachToComponent(GetRootComponent());
  CollisionComponent->SetStatic(false);
  CollisionComponent->RegisterComponent();

  Movement = NewObject<MNetMovementComponent>(this);
  Movement->RegisterComponent();

  ReplicationTest = NewObject<MNetworkTestRepComponent>(this);
  ReplicationTest->RegisterComponent();
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
  if (!bIsLocallyControlled || !Movement) {
    return;
  }

  Movement->AddMovementInput(Value.Axis2D);
}

void ANetworkTestPawn::OnInteract(const FInputActionValue& Value) {
  (void)Value;

  if (!bIsLocallyControlled) {
    return;
  }
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

  if (auto OtherPawn = dynamic_cast<ANetworkTestPawn*>(OtherActor)) {
    M_LOG(
        "Player overlap detected! My NetworkId: {}, Other NetworkId: {}",
        NetworkId,
        OtherPawn->NetworkId
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
