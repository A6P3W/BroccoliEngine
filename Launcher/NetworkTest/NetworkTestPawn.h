#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "ActorComponent.h"
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;
class MCharacterMovementComponent;
class FNetworkTestUI;

class MNetworkTestRepComponent : public MActorComponent {
 public:
  MNetworkTestRepComponent();

  void RequestTest(int PlayerId);
  int GetReplicatedCounter() const { return ReplicatedCounter; }
  bool IsOnRepFlashActive() const { return OnRepFlashTimer > 0.0f; }
  bool IsRPCFlashActive() const { return RPCFlashTimer > 0.0f; }

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  void Server_ComponentTest(int PlayerId);
  void Multicast_ComponentTest(int PlayerId, int CounterValue);
  void OnRepReplicatedCounter(int OldValue);

  int ReplicatedCounter = 0;
  float OnRepFlashTimer = 0.0f;
  float RPCFlashTimer = 0.0f;
  float PendingOnRepFlashTimer = 0.0f;
};

class ANetworkTestPawn : public APawn {
 public:
  DEFINE_ACTOR_CLASS(ANetworkTestPawn)
  ANetworkTestPawn();
  ~ANetworkTestPawn() override;

  void BeginPlay() override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUpdate(float DeltaTime) override;
  void Draw() override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

  void SetStatusMessage(const std::string& Message);
  bool StartListenServer(uint16_t Port);
  bool ConnectAsClient(const std::string& HostAddress, uint16_t Port);

 private:
  void OnMove(const FInputActionValue& Value);
  void OnInteract(const FInputActionValue& Value);
  void BeginOverlap(AActor* OtherActor) override;
  int GetDisplayColor() const;

  MSpriteComponent* BodySprite = nullptr;
  MCharacterMovementComponent* Movement = nullptr;
  MNetworkTestRepComponent* ReplicationTest = nullptr;
  std::unique_ptr<FNetworkTestUI> NetworkTestUI;
  float FlashTimer = 0.0f;

  bool bSessionStarted = false;
};
