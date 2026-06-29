#include "NetworkTest/NetworkTestPawn.h"

#include <DxLib.h>
#include <MovementComponent.h>
#include <RectangleCollisionComponent.h>
#include <imgui.h>

#include "EnhancedInputComponent.h"
#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "Log.h"
#include "NetworkManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
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

void ANetworkTestPawn::BeginPlay() {
  APawn::BeginPlay();
  if (bIsLocallyControlled) {
    StatusMessage = "Select network role.";
    if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(GetWorld()->GetGameMode())) {
      StatusMessage = std::string(gameMode->GetSceneName()) + " loaded.";
    }
    OnlineStatusMessage = EOSCoreManager::Get().IsInitialized()
                              ? "EOS initialized. Login to use lobby."
                              : "EOS is not initialized.";
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
  if (bIsLocallyControlled) {
    DrawConnectionWindow();
    DrawOnlineWindow();
    DrawStatusWindow();
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
  if (bIsLocallyControlled) {
    StatusMessage = Message;
  }
}

void ANetworkTestPawn::DrawConnectionWindow() {
  ImGui::Begin("Network Test");
  ImGui::TextUnformatted("Local Network Replication Test");
  ImGui::Separator();

  ImGui::InputInt("Port", &Port);
  if (Port < 1) {
    Port = 1;
  }
  if (Port > 65535) {
    Port = 65535;
  }

  if (ImGui::Button("Start Listen Server")) {
    StartListenServer();
  }

  ImGui::InputText("Server IP", ServerAddress, sizeof(ServerAddress));
  if (ImGui::Button("Connect as Client")) {
    ConnectAsClient();
  }

  ImGui::Separator();

  ANetworkTestGameMode* gameMode = dynamic_cast<ANetworkTestGameMode*>(GetWorld()->GetGameMode());
  const char* sceneName = gameMode ? gameMode->GetSceneName() : "Unknown Scene";
  ImGui::Text("Current Scene: %s", sceneName);

  if (GetWorld()->IsServer() && gameMode && ImGui::Button(gameMode->GetTravelButtonText())) {
    if (GetWorld()->ServerTravel(gameMode->GetTravelTargetSceneId())) {
      StatusMessage = "Server travel requested.";
    } else {
      StatusMessage = "Server travel failed. Scene is not registered or local role is not server.";
    }
  }
  ImGui::Separator();
  ImGui::TextWrapped("%s", StatusMessage.c_str());
  ImGui::End();
}

void ANetworkTestPawn::DrawOnlineWindow() {
  EOSCoreManager& coreManager = EOSCoreManager::Get();
  EOSAuthManager& authManager = EOSAuthManager::Get();
  EOSLobbyManager& lobbyManager = EOSLobbyManager::Get();

  if (OnlineLobbyMaxMembers < 1) {
    OnlineLobbyMaxMembers = 1;
  }
  if (OnlineLobbyMaxMembers > 64) {
    OnlineLobbyMaxMembers = 64;
  }
  if (OnlineSearchMaxResults < 1) {
    OnlineSearchMaxResults = 1;
  }
  if (OnlineSearchMaxResults > 100) {
    OnlineSearchMaxResults = 100;
  }

  ImGui::Begin("EOS Lobby Test");
  ImGui::Text("EOS initialized: %s", coreManager.IsInitialized() ? "true" : "false");
  ImGui::Text("Logged in: %s", authManager.IsLoggedIn() ? "true" : "false");
  if (authManager.IsLoggedIn()) {
    const std::string localUserId = authManager.GetLocalUserIdString();
    ImGui::TextWrapped("ProductUserId: %s", localUserId.c_str());
  }
  ImGui::Text("In lobby: %s", lobbyManager.IsInLobby() ? "true" : "false");
  if (lobbyManager.IsInLobby()) {
    const std::string lobbyId = lobbyManager.GetCurrentLobbyId();
    ImGui::TextWrapped("Current LobbyId: %s", lobbyId.c_str());
  }
  ImGui::Text("Operation pending: %s", bOnlineOperationPending ? "true" : "false");
  ImGui::Separator();

  ImGui::InputText("Display Name", OnlineDisplayName, sizeof(OnlineDisplayName));
  if (ImGui::Button("Login Device ID")) {
    LoginWithDeviceId();
  }

  ImGui::Separator();
  ImGui::InputInt("Max Members", &OnlineLobbyMaxMembers);
  if (ImGui::Button("Create Lobby")) {
    CreateOnlineLobby();
  }

  ImGui::InputInt("Search Max Results", &OnlineSearchMaxResults);
  if (ImGui::Button("Search Lobbies")) {
    SearchOnlineLobbies();
  }

  if (!OnlineSearchResults.empty()) {
    ImGui::BeginChild("EOSLobbySearchResults", ImVec2(0.0f, 120.0f), true);
    for (int index = 0; index < static_cast<int>(OnlineSearchResults.size()); ++index) {
      const FLobbyInfo& lobbyInfo = OnlineSearchResults[index];
      std::string label = std::to_string(index + 1) + ": " + lobbyInfo.LobbyId + " (" +
                          std::to_string(lobbyInfo.CurrentMembers) + "/" +
                          std::to_string(lobbyInfo.MaxMembers) + ")";
      if (ImGui::Selectable(label.c_str(), SelectedOnlineLobbyIndex == index)) {
        SelectedOnlineLobbyIndex = index;
      }
    }
    ImGui::EndChild();
  } else {
    ImGui::TextDisabled("No lobby search results.");
  }

  if (ImGui::Button("Join Selected Lobby")) {
    JoinSelectedOnlineLobby();
  }
  if (ImGui::Button("Leave Lobby")) {
    LeaveOnlineLobby();
  }

  ImGui::Separator();
  ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
  ImGui::End();
}
void ANetworkTestPawn::DrawStatusWindow() {
  ImGui::Begin("Network Status");

  const char* modeText = "Standalone";
  if (GetWorld()->IsListenServer()) {
    modeText = "ListenServer";
  } else if (GetWorld()->IsClient()) {
    modeText = "Client";
  }

  NetworkManager& network = NetworkManager::GetInstance();
  ImGui::Text("Mode: %s", modeText);
  ImGui::Text("Network running: %s", network.IsRunning() ? "true" : "false");
  ImGui::Text("Local ConnectionId: %u", network.GetLocalConnectionId());

  bool bHostSpawned = false;
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(GetWorld()->GetGameMode())) {
    bHostSpawned = gameMode->IsHostPlayerSpawned();
  }
  ImGui::Text("Host player spawned: %s", bHostSpawned ? "true" : "false");
  ImGui::End();
}

void ANetworkTestPawn::StartListenServer() {
  if (NetworkManager::GetInstance().StartServer(static_cast<uint16_t>(Port))) {
    GetWorld()->SetNetMode(ENetMode::ListenServer);
    bSessionStarted = true;
    StatusMessage = "Listen server started. Host player will spawn on next update.";
  } else {
    StatusMessage = "Failed to start listen server.";
  }
}

void ANetworkTestPawn::ConnectAsClient() {
  if (NetworkManager::GetInstance().ConnectToServer(ServerAddress, static_cast<uint16_t>(Port))) {
    GetWorld()->SetNetMode(ENetMode::Client);
    bSessionStarted = true;
    StatusMessage = "Connecting to server...";
  } else {
    StatusMessage = "Failed to connect to server.";
  }
}

void ANetworkTestPawn::LoginWithDeviceId() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSCoreManager::Get().IsInitialized()) {
    OnlineStatusMessage = "EOS is not initialized.";
    return;
  }

  if (EOSAuthManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Already logged in.";
    return;
  }

  bOnlineOperationPending = true;
  OnlineStatusMessage = "EOS Device ID login requested.";
  EOSAuthManager::Get().LoginWithDeviceId(OnlineDisplayName, [this](bool bSuccess) {
    bOnlineOperationPending = false;
    if (bSuccess) {
      OnlineStatusMessage = "EOS Device ID login succeeded.";
      const std::string localUserId = EOSAuthManager::Get().GetLocalUserIdString();
      if (!localUserId.empty()) {
        OnlineStatusMessage += " ProductUserId: " + localUserId;
      }
    } else {
      OnlineStatusMessage = "EOS Device ID login failed. See Logs for details.";
    }
  });
}

void ANetworkTestPawn::CreateOnlineLobby() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSAuthManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before creating a lobby.";
    return;
  }

  if (EOSLobbyManager::Get().IsInLobby()) {
    OnlineStatusMessage = "Already in a lobby. Leave it before creating another one.";
    return;
  }

  if (OnlineLobbyMaxMembers < 1) {
    OnlineLobbyMaxMembers = 1;
  }

  FCreateLobbyRequest request;
  request.MaxMembers = OnlineLobbyMaxMembers;
  request.bPublicAdvertised = true;

  bOnlineOperationPending = true;
  OnlineStatusMessage = "Create lobby requested.";
  EOSLobbyManager::Get().CreateLobby(request, [this](bool bSuccess, const FLobbyInfo& lobbyInfo) {
    bOnlineOperationPending = false;
    if (bSuccess) {
      OnlineSearchResults.clear();
      SelectedOnlineLobbyIndex = -1;
      OnlineStatusMessage = "Lobby created: " + lobbyInfo.LobbyId;
    } else {
      OnlineStatusMessage = "Create lobby failed. See Logs for details.";
    }
  });
}

void ANetworkTestPawn::SearchOnlineLobbies() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSAuthManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before searching lobbies.";
    return;
  }

  if (OnlineSearchMaxResults < 1) {
    OnlineSearchMaxResults = 1;
  }

  FLobbySearchRequest request;
  request.MaxResults = OnlineSearchMaxResults;

  bOnlineOperationPending = true;
  OnlineSearchResults.clear();
  SelectedOnlineLobbyIndex = -1;
  OnlineStatusMessage = "Search lobbies requested.";
  EOSLobbyManager::Get().SearchLobbies(
      request,
      [this](bool bSuccess, const std::vector<FLobbyInfo>& results) {
        bOnlineOperationPending = false;
        if (bSuccess) {
          OnlineSearchResults = results;
          SelectedOnlineLobbyIndex = OnlineSearchResults.empty() ? -1 : 0;
          OnlineStatusMessage = "Lobby search completed. Found: " + std::to_string(OnlineSearchResults.size());
        } else {
          OnlineStatusMessage = "Lobby search failed. See Logs for details.";
        }
      }
  );
}

void ANetworkTestPawn::JoinSelectedOnlineLobby() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSAuthManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before joining a lobby.";
    return;
  }

  if (EOSLobbyManager::Get().IsInLobby()) {
    OnlineStatusMessage = "Already in a lobby. Leave it before joining another one.";
    return;
  }

  if (SelectedOnlineLobbyIndex < 0 ||
      SelectedOnlineLobbyIndex >= static_cast<int>(OnlineSearchResults.size())) {
    OnlineStatusMessage = "Select a lobby search result first.";
    return;
  }

  const FLobbyInfo lobbyInfo = OnlineSearchResults[SelectedOnlineLobbyIndex];
  bOnlineOperationPending = true;
  OnlineStatusMessage = "Join lobby requested: " + lobbyInfo.LobbyId;
  EOSLobbyManager::Get().JoinLobby(lobbyInfo, [this, lobbyInfo](bool bSuccess) {
    bOnlineOperationPending = false;
    if (bSuccess) {
      OnlineStatusMessage = "Joined lobby: " + lobbyInfo.LobbyId;
    } else {
      OnlineStatusMessage = "Join lobby failed. Search again if the result is stale.";
    }
  });
}

void ANetworkTestPawn::LeaveOnlineLobby() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSLobbyManager::Get().IsInLobby()) {
    OnlineStatusMessage = "Not in a lobby.";
    return;
  }

  const std::string lobbyId = EOSLobbyManager::Get().GetCurrentLobbyId();
  bOnlineOperationPending = true;
  OnlineStatusMessage = "Leave lobby requested: " + lobbyId;
  EOSLobbyManager::Get().LeaveLobby([this, lobbyId](bool bSuccess) {
    bOnlineOperationPending = false;
    if (bSuccess) {
      OnlineStatusMessage = "Left lobby: " + lobbyId;
    } else {
      OnlineStatusMessage = "Leave lobby failed. See Logs for details.";
    }
  });
}
