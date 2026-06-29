#pragma once

#include "EOSTypes.h"
#include "ActorComponent.h"
#include "Pawn.h"
#include "UMath.h"

#include <string>
#include <vector>

class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;
class MMovementComponent;

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

  void BeginPlay() override;
  void OnPossessedBy(APlayerController* NewController) override;
  void OnUpdate(float DeltaTime) override;
  void Draw() override;
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

  void SetStatusMessage(const std::string& Message);

 private:
  void OnMove(const FInputActionValue& Value);
  void OnInteract(const FInputActionValue& Value);
  void Server_TestRPC(int PlayerId);
  void Multicast_TestRPC(int PlayerId);
  void Server_Move(const FVector2D& MoveInput);
  void ApplyMovementInput(const FVector2D& MoveInput);
  void BeginOverlap(AActor* OtherActor) override;
  int GetDisplayColor() const;

  void DrawConnectionWindow();
  void DrawOnlineWindow();
  void DrawStatusWindow();
  void StartListenServer();
  void ConnectAsClient();
  void LoginWithDeviceId();
  void CreateOnlineLobby();
  void SearchOnlineLobbies();
  void JoinSelectedOnlineLobby();
  void LeaveOnlineLobby();

  MSpriteComponent* BodySprite = nullptr;
  MMovementComponent* Movement = nullptr;
  MNetworkTestRepComponent* ReplicationTest = nullptr;
  float MoveSpeed = 320.0f;
  float FlashTimer = 0.0f;

  char ServerAddress[64] = "127.0.0.1";
  int Port = 7777;
  bool bSessionStarted = false;
  std::string StatusMessage;

  char OnlineDisplayName[32] = "BroccoliPlayer";
  int OnlineLobbyMaxMembers = 4;
  int OnlineSearchMaxResults = 10;
  bool bOnlineOperationPending = false;
  std::vector<FLobbyInfo> OnlineSearchResults;
  int SelectedOnlineLobbyIndex = -1;
  std::string OnlineStatusMessage;
};
