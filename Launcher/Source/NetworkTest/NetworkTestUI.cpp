#include "NetworkTestUI.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "NetworkManager.h"
#include "NetworkTestGameMode.h"
#include "NetworkTestPawn.h"
#include "NetworkUtils.h"
#include "OnlineSessionManager.h"
#include "World.h"

FNetworkTestUI::FNetworkTestUI(ANetworkTestPawn& InOwner) : Owner(InOwner) {}

void FNetworkTestUI::InitializeStatus() {
  StatusMessage = "Select network role.";
  // 現在のワールドから稼働しているゲームモード（ANetworkTestGameMode）を取得
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    StatusMessage = std::string(gameMode->GetSceneName()) + " loaded.";
  }

  // Epic Online Services (EOS) が初期化されているかどうかを判定
  OnlineStatusMessage = OnlineSessionManager::Get().IsEOSInitialized()
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

  bool RequestedEOSP2P = UseEOSP2P;
  if (NetworkManager::GetInstance().IsRunning()) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Checkbox("Use EOS P2P", &RequestedEOSP2P)) {
    const ENetworkTransportType RequestedType =
        RequestedEOSP2P ? ENetworkTransportType::EOSP2P : ENetworkTransportType::ENet;
    if (NetworkManager::GetInstance().SetTransportType(RequestedType)) {
      UseEOSP2P = RequestedEOSP2P;
      StatusMessage =
          UseEOSP2P ? "EOS P2P selected. Login before starting or connecting." : "ENet selected.";
    }
  }
  if (NetworkManager::GetInstance().IsRunning()) {
    ImGui::EndDisabled();
  }
  ImGui::TextDisabled(
      UseEOSP2P ? "Client target: host Product User ID" : "Client target: server IP"
  );

  ImGui::InputInt("Port", &Port);
  Port = std::clamp(Port, 1, 65535);

  if (ImGui::Button("Start Listen Server")) {
    if (Owner.StartListenServer(static_cast<uint16_t>(Port))) {
      StatusMessage = "Listen server started. Host player will spawn on next update.";
    } else {
      StatusMessage = "Failed to start listen server.";
    }
  }

  ImGui::InputText(
      UseEOSP2P ? "Host Product User ID" : "Server IP", ServerAddress, sizeof(ServerAddress)
  );
  if (ImGui::Button("Connect as Client")) {
    if (Owner.ConnectAsClient(ServerAddress, static_cast<uint16_t>(Port))) {
      StatusMessage = "Connecting to server...";
    } else {
      StatusMessage = "Failed to connect to server.";
    }
  }

  ImGui::Separator();

  ANetworkTestGameMode* gameMode =
      dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode());
  const char* sceneName = gameMode ? gameMode->GetSceneName() : "Unknown Scene";
  ImGui::Text("Current Scene: %s", sceneName);

  // サーバー権限を持つ（IsServer()）かつ、ゲームモードが存在する場合のみトラベル（レベル移行）処理を実行可能にする
  if (Owner.GetWorld()->IsServer() && gameMode && ImGui::Button(gameMode->GetTravelButtonText())) {
    // 接続している全クライアントを連れて、指定のレベルへ移行（トラベル）するエンジンの機能
    if (Owner.GetWorld()->ServerTravel(gameMode->GetTravelTargetLevelPath())) {
      StatusMessage = "Server travel requested.";
    } else {
      StatusMessage = "Server travel failed. Level path is empty or local role is not server.";
    }
  }
  ImGui::Separator();
  ImGui::TextWrapped("%s", StatusMessage.c_str());
  ImGui::End();
}

void FNetworkTestUI::DrawOnlineWindow() {
  // EOS を統合管理するシングルトンマネージャーを取得
  OnlineSessionManager& onlineSession = OnlineSessionManager::Get();

  OnlineLobbyMaxMembers = std::clamp(OnlineLobbyMaxMembers, 1, 64);
  OnlineSearchMaxResults = std::clamp(OnlineSearchMaxResults, 1, 100);

  ImGui::Begin("EOS LAN Lobby Test");
  // EOS の初期化、ログイン状態、現在ロビー参加中かどうかを取得して UI に描画
  ImGui::Text("EOS initialized: %s", onlineSession.IsEOSInitialized() ? "true" : "false");
  ImGui::Text("Logged in: %s", onlineSession.IsLoggedIn() ? "true" : "false");
  if (onlineSession.IsLoggedIn()) {
    // ログイン中のユーザーID（ProductUserId）を取得
    const std::string localUserId = onlineSession.GetLocalUserIdString();
    ImGui::TextWrapped("ProductUserId: %s", localUserId.c_str());
  }
  ImGui::Text("In lobby: %s", onlineSession.IsInLobby() ? "true" : "false");
  if (onlineSession.IsInLobby()) {
    // 現在のロビー ID を取得
    const std::string lobbyId = onlineSession.GetCurrentLobbyId();
    ImGui::TextWrapped("Current LobbyId: %s", lobbyId.c_str());
  }
  // 非同期の EOS API が現在進行中か（処理中か）を取得
  ImGui::Text("Operation pending: %s", onlineSession.IsOperationPending() ? "true" : "false");
  ImGui::Separator();

  if (!onlineSession.IsEOSInitialized()) {
    ImGui::TextDisabled("EOS is not initialized.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (!onlineSession.IsLoggedIn()) {
    ImGui::InputText("Display Name", OnlineDisplayName, sizeof(OnlineDisplayName));
    if (onlineSession.CanShowLogin() && ImGui::Button("Login Device ID")) {
      LoginWithDeviceId();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (onlineSession.IsInLobby()) {
    if (onlineSession.CanShowLeaveSession() && ImGui::Button("Leave Session")) {
      LeaveOnlineSession();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (onlineSession.IsOperationPending()) {
    ImGui::TextDisabled("EOS operation is pending.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  ImGui::InputInt("Max Members", &OnlineLobbyMaxMembers);
  if (onlineSession.CanShowLobbyActions() && ImGui::Button("Create LAN Lobby")) {
    CreateLanLobby();
  }

  ImGui::InputInt("Search Max Results", &OnlineSearchMaxResults);
  if (onlineSession.CanShowSearchLobbies() && ImGui::Button("Search Lobbies")) {
    SearchOnlineLobbies();
  }

  if (!OnlineSearchResults.empty()) {
    ImGui::BeginChild("EOSLobbySearchResults", ImVec2(0.0f, 120.0f), true);
    for (int index = 0; index < static_cast<int>(OnlineSearchResults.size()); ++index) {
      const FLobbyInfo& lobbyInfo = OnlineSearchResults[index];
      std::string label =
          std::to_string(index + 1) + ": " + lobbyInfo.LobbyId + " (" +
          std::to_string(lobbyInfo.CurrentMembers) + "/" + std::to_string(lobbyInfo.MaxMembers) +
          ") HostIP=" + (lobbyInfo.HostIPAddress.empty() ? "<none>" : lobbyInfo.HostIPAddress);
      if (ImGui::Selectable(label.c_str(), SelectedOnlineLobbyIndex == index)) {
        SelectedOnlineLobbyIndex = index;
      }
    }
    ImGui::EndChild();

    if (onlineSession.CanShowJoinLobby() && ImGui::Button("Join Selected LAN Lobby")) {
      JoinSelectedLanLobby();
    }
  } else {
    ImGui::TextDisabled("No lobby search results.");
  }

  ImGui::Separator();
  ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
  ImGui::End();
}
void FNetworkTestUI::DrawStatusWindow() {
  ImGui::Begin("Network Status");

  const char* modeText = "Standalone";
  // ワールドが ListenServer（サーバー＆クライアント）か、Client かを判定して表記を分ける
  if (Owner.GetWorld()->IsListenServer()) {
    modeText = "ListenServer";
  } else if (Owner.GetWorld()->IsClient()) {
    modeText = "Client";
  }

  // ネットワーク処理の低レイヤーマネージャーを取得
  NetworkManager& network = NetworkManager::GetInstance();
  ImGui::Text("Mode: %s", modeText);
  // ネットワーク接続が現在稼働しているかを取得
  ImGui::Text("Network running: %s", network.IsRunning() ? "true" : "false");
  // ローカル環境自身のネットワーク接続IDを取得（未接続・単体起動時は無効値）
  ImGui::Text("Local ConnectionId: %u", network.GetLocalConnectionId());

  bool bHostSpawned = false;
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    // サーバーのホストプレイヤー（ホスト Pawn）がワールド上に生成済みか判定
    bHostSpawned = gameMode->IsHostPlayerSpawned();
  }
  ImGui::Text("Host player spawned: %s", bHostSpawned ? "true" : "false");
  ImGui::End();
}

void FNetworkTestUI::LoginWithDeviceId() {
  if (!OnlineSessionManager::Get().IsEOSInitialized()) {
    OnlineStatusMessage = "EOS is not initialized.";
    return;
  }

  if (OnlineSessionManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Already logged in.";
    return;
  }

  OnlineStatusMessage = "EOS Device ID login requested.";
  // EOS Connect サービスのログイン API を実行（引数にユーザー名と成功可否を受け取るコールバックを渡す）
  if (!OnlineSessionManager::Get().LoginWithDeviceId(OnlineDisplayName, [this](bool bSuccess) {
        if (bSuccess) {
          OnlineStatusMessage = "EOS Device ID login succeeded.";
          const std::string localUserId = OnlineSessionManager::Get().GetLocalUserIdString();
          if (!localUserId.empty()) {
            OnlineStatusMessage += " ProductUserId: " + localUserId;
          }
        } else {
          OnlineStatusMessage = "EOS Device ID login failed. See Logs for details.";
        }
      })) {
    OnlineStatusMessage = "EOS Device ID login request was rejected.";
  }
}

void FNetworkTestUI::CreateLanLobby() {
  if (!OnlineSessionManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before creating a lobby.";
    return;
  }

  if (OnlineSessionManager::Get().IsInLobby()) {
    OnlineStatusMessage = "Already in a lobby. Leave it before creating another one.";
    return;
  }

  // ローカルPCのプライベートIPアドレスを取得するユーティリティ関数
  const std::string localIPAddress = NetworkUtils::GetLocalIPAddress();
  if (localIPAddress.empty()) {
    OnlineStatusMessage = "Local IP address was not found.";
    return;
  }

  FCreateLobbyRequest request;
  request.MaxMembers = (std::max)(1, OnlineLobbyMaxMembers);
  request.bPublicAdvertised = true;
  request.HostIPAddress = localIPAddress;

  OnlineStatusMessage = "Create LAN lobby requested. HostIP=" + localIPAddress;
  // 新しい EOS ロビーの作成を非同期でリクエスト
  if (!OnlineSessionManager::Get().CreateLobby(
          request, [this](bool bSuccess, const FLobbyInfo& lobbyInfo) {
            if (!bSuccess) {
              OnlineStatusMessage = "Create LAN lobby failed. See Logs for details.";
              return;
            }

            OnlineSearchResults.clear();
            SelectedOnlineLobbyIndex = -1;
            // ロビー作成成功後、自アクター（ホスト側）でリスンサーバーを起動して接続の待受を開始
            if (Owner.StartListenServer(static_cast<uint16_t>(Port))) {
              StatusMessage = "Listen server started from EOS lobby.";
              OnlineStatusMessage =
                  "LAN lobby created: " + lobbyInfo.LobbyId + " HostIP=" + lobbyInfo.HostIPAddress;
            } else {
              OnlineStatusMessage = "LAN lobby created, but listen server failed to start.";
              StatusMessage = "Failed to start listen server.";
            }
          }
      )) {
    OnlineStatusMessage = "Create LAN lobby request was rejected.";
  }
}

void FNetworkTestUI::SearchOnlineLobbies() {
  if (!OnlineSessionManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before searching lobbies.";
    return;
  }

  FLobbySearchRequest request;
  request.MaxResults = (std::max)(1, OnlineSearchMaxResults);

  OnlineSearchResults.clear();
  SelectedOnlineLobbyIndex = -1;
  OnlineStatusMessage = "Search lobbies requested.";
  // 作成されているアクティブな EOS ロビーの検索を非同期で実行
  if (!OnlineSessionManager::Get().SearchLobbies(
          request, [this](bool bSuccess, const std::vector<FLobbyInfo>& results) {
            if (bSuccess) {
              OnlineSearchResults = results;
              SelectedOnlineLobbyIndex = OnlineSearchResults.empty() ? -1 : 0;
              OnlineStatusMessage =
                  "Lobby search completed. Found: " + std::to_string(OnlineSearchResults.size());
            } else {
              OnlineStatusMessage = "Lobby search failed. See Logs for details.";
            }
          }
      )) {
    OnlineStatusMessage = "Lobby search request was rejected.";
  }
}

void FNetworkTestUI::JoinSelectedLanLobby() {
  if (!OnlineSessionManager::Get().IsLoggedIn()) {
    OnlineStatusMessage = "Login before joining a lobby.";
    return;
  }

  if (OnlineSessionManager::Get().IsInLobby()) {
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

  OnlineStatusMessage = "Join lobby requested: " + lobbyInfo.LobbyId;
  // 選択した EOS ロビーへの参加およびホストへのネットワーク接続をリクエスト
  if (!OnlineSessionManager::Get().JoinLobby(
          lobbyInfo, static_cast<uint16_t>(Port), [this, lobbyInfo](bool bSuccess) {
            if (!bSuccess) {
              OnlineStatusMessage =
                  "Join lobby or ENet connection failed. Search again if the result is stale.";
              return;
            }

            strncpy_s(
                ServerAddress, sizeof(ServerAddress), lobbyInfo.HostIPAddress.c_str(), _TRUNCATE
            );
            // 接続成功後、ワールドのネットワークモードを Client に設定
            Owner.GetWorld()->SetNetMode(ENetMode::Client);
            StatusMessage = "Connecting to lobby host: " + lobbyInfo.HostIPAddress;
            OnlineStatusMessage = "Joined lobby and connecting: " + lobbyInfo.LobbyId;
          }
      )) {
    OnlineStatusMessage = "Join lobby request was rejected.";
  }
}
void FNetworkTestUI::LeaveOnlineSession() {
  if (!OnlineSessionManager::Get().IsInLobby()) {
    OnlineStatusMessage = "Not in a lobby.";
    return;
  }

  const std::string lobbyId = OnlineSessionManager::Get().GetCurrentLobbyId();
  OnlineStatusMessage = "Leave session requested: " + lobbyId;
  // 現在参加しているマルチプレイロビーから退出
  if (!OnlineSessionManager::Get().LeaveSession([this, lobbyId](bool bSuccess) {
        if (bSuccess) {
          // セッション退出成功後、接続を切断しネットワークモードを Standalone（未接続状態）へ戻す
          Owner.GetWorld()->SetNetMode(ENetMode::Standalone);
          OnlineStatusMessage = "Left session: " + lobbyId;
        } else {
          OnlineStatusMessage = "Leave session failed. See Logs for details.";
        }
      })) {
    OnlineStatusMessage = "Leave session request was rejected.";
  }
}
