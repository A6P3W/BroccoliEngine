#include "NetworkTest/NetworkTestUI.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <imgui.h>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "NetworkManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
#include "NetworkTest/NetworkTestPawn.h"
#include "NetworkUtils.h"
#include "World.h"

FNetworkTestUI::FNetworkTestUI(ANetworkTestPawn& InOwner) : Owner(InOwner) {}

void FNetworkTestUI::InitializeStatus() {
  StatusMessage = "Select network role.";
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    StatusMessage = std::string(gameMode->GetSceneName()) + " loaded.";
  }

  OnlineStatusMessage = EOSCoreManager::Get().IsInitialized()
                            ? "EOS initialized. Login to use lobby."
                            : "EOS is not initialized.";
}

void FNetworkTestUI::Draw() {
  DrawConnectionWindow();
  DrawOnlineWindow();
  DrawStatusWindow();
}

void FNetworkTestUI::SetStatusMessage(const std::string& Message) { StatusMessage = Message; }

void FNetworkTestUI::DrawConnectionWindow() {
  ImGui::Begin("Network Test");
  ImGui::TextUnformatted("Local Network Replication Test");
  ImGui::Separator();

  ImGui::InputInt("Port", &Port);
  Port = std::clamp(Port, 1, 65535);

  if (ImGui::Button("Start Listen Server")) {
    if (Owner.StartListenServer(static_cast<uint16_t>(Port))) {
      StatusMessage = "Listen server started. Host player will spawn on next update.";
    } else {
      StatusMessage = "Failed to start listen server.";
    }
  }

  ImGui::InputText("Server IP", ServerAddress, sizeof(ServerAddress));
  if (ImGui::Button("Connect as Client")) {
    if (Owner.ConnectAsClient(ServerAddress, static_cast<uint16_t>(Port))) {
      StatusMessage = "Connecting to server...";
    } else {
      StatusMessage = "Failed to connect to server.";
    }
  }

  ImGui::Separator();

  ANetworkTestGameMode* gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode());
  const char* sceneName = gameMode ? gameMode->GetSceneName() : "Unknown Scene";
  ImGui::Text("Current Scene: %s", sceneName);

  if (Owner.GetWorld()->IsServer() && gameMode && ImGui::Button(gameMode->GetTravelButtonText())) {
    if (Owner.GetWorld()->ServerTravel(gameMode->GetTravelTargetSceneId())) {
      StatusMessage = "Server travel requested.";
    } else {
      StatusMessage = "Server travel failed. Scene is not registered or local role is not server.";
    }
  }
  ImGui::Separator();
  ImGui::TextWrapped("%s", StatusMessage.c_str());
  ImGui::End();
}

void FNetworkTestUI::DrawOnlineWindow() {
  EOSCoreManager& coreManager = EOSCoreManager::Get();
  EOSAuthManager& authManager = EOSAuthManager::Get();
  EOSLobbyManager& lobbyManager = EOSLobbyManager::Get();

  OnlineLobbyMaxMembers = std::clamp(OnlineLobbyMaxMembers, 1, 64);
  OnlineSearchMaxResults = std::clamp(OnlineSearchMaxResults, 1, 100);

  ImGui::Begin("EOS LAN Lobby Test");
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
  if (ImGui::Button("Create LAN Lobby")) {
    CreateLanLobby();
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
                          std::to_string(lobbyInfo.MaxMembers) + ") HostIP=" +
                          (lobbyInfo.HostIPAddress.empty() ? "<none>" : lobbyInfo.HostIPAddress);
      if (ImGui::Selectable(label.c_str(), SelectedOnlineLobbyIndex == index)) {
        SelectedOnlineLobbyIndex = index;
      }
    }
    ImGui::EndChild();
  } else {
    ImGui::TextDisabled("No lobby search results.");
  }

  if (ImGui::Button("Join Selected LAN Lobby")) {
    JoinSelectedLanLobby();
  }
  if (ImGui::Button("Leave Lobby")) {
    LeaveOnlineLobby();
  }

  ImGui::Separator();
  ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
  ImGui::End();
}

void FNetworkTestUI::DrawStatusWindow() {
  ImGui::Begin("Network Status");

  const char* modeText = "Standalone";
  if (Owner.GetWorld()->IsListenServer()) {
    modeText = "ListenServer";
  } else if (Owner.GetWorld()->IsClient()) {
    modeText = "Client";
  }

  NetworkManager& network = NetworkManager::GetInstance();
  ImGui::Text("Mode: %s", modeText);
  ImGui::Text("Network running: %s", network.IsRunning() ? "true" : "false");
  ImGui::Text("Local ConnectionId: %u", network.GetLocalConnectionId());

  bool bHostSpawned = false;
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    bHostSpawned = gameMode->IsHostPlayerSpawned();
  }
  ImGui::Text("Host player spawned: %s", bHostSpawned ? "true" : "false");
  ImGui::End();
}

void FNetworkTestUI::LoginWithDeviceId() {
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

void FNetworkTestUI::CreateLanLobby() {
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

  const std::string localIPAddress = NetworkUtils::GetLocalIPAddress();
  if (localIPAddress.empty()) {
    OnlineStatusMessage = "Local IP address was not found.";
    return;
  }

  FCreateLobbyRequest request;
  request.MaxMembers = (std::max)(1, OnlineLobbyMaxMembers);
  request.bPublicAdvertised = true;
  request.HostIPAddress = localIPAddress;

  bOnlineOperationPending = true;
  OnlineStatusMessage = "Create LAN lobby requested. HostIP=" + localIPAddress;
  EOSLobbyManager::Get().CreateLobby(request, [this](bool bSuccess, const FLobbyInfo& lobbyInfo) {
    bOnlineOperationPending = false;
    if (!bSuccess) {
      OnlineStatusMessage = "Create LAN lobby failed. See Logs for details.";
      return;
    }

    OnlineSearchResults.clear();
    SelectedOnlineLobbyIndex = -1;
    if (Owner.StartListenServer(static_cast<uint16_t>(Port))) {
      StatusMessage = "Listen server started from EOS lobby.";
      OnlineStatusMessage = "LAN lobby created: " + lobbyInfo.LobbyId + " HostIP=" + lobbyInfo.HostIPAddress;
    } else {
      OnlineStatusMessage = "LAN lobby created, but listen server failed to start.";
      StatusMessage = "Failed to start listen server.";
    }
  });
}

void FNetworkTestUI::SearchOnlineLobbies() {
  if (bOnlineOperationPending) {
    OnlineStatusMessage = "EOS operation is already pending.";
    return;
  }

  if (!EOSAuthManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before searching lobbies.";
    return;
  }

  FLobbySearchRequest request;
  request.MaxResults = (std::max)(1, OnlineSearchMaxResults);

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
          OnlineStatusMessage =
              "Lobby search completed. Found: " + std::to_string(OnlineSearchResults.size());
        } else {
          OnlineStatusMessage = "Lobby search failed. See Logs for details.";
        }
      }
  );
}

void FNetworkTestUI::JoinSelectedLanLobby() {
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
  if (lobbyInfo.HostIPAddress.empty()) {
    OnlineStatusMessage = "Selected lobby does not have HostIP.";
    return;
  }

  bOnlineOperationPending = true;
  OnlineStatusMessage = "Join lobby requested: " + lobbyInfo.LobbyId;
  EOSLobbyManager::Get().JoinLobby(lobbyInfo, [this, lobbyInfo](bool bSuccess) {
    bOnlineOperationPending = false;
    if (!bSuccess) {
      OnlineStatusMessage = "Join lobby failed. Search again if the result is stale.";
      return;
    }

    strncpy_s(ServerAddress, sizeof(ServerAddress), lobbyInfo.HostIPAddress.c_str(), _TRUNCATE);

    if (Owner.ConnectAsClient(lobbyInfo.HostIPAddress, static_cast<uint16_t>(Port))) {
      StatusMessage = "Connecting to lobby host: " + lobbyInfo.HostIPAddress;
      OnlineStatusMessage = "Joined lobby and connecting: " + lobbyInfo.LobbyId;
    } else {
      OnlineStatusMessage = "Joined lobby, but ENet connection failed.";
      StatusMessage = "Failed to connect to server.";
    }
  });
}

void FNetworkTestUI::LeaveOnlineLobby() {
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
