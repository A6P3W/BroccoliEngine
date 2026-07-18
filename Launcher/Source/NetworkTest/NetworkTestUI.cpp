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
  // NetworkManager::GetInstance(): ネットワーク制御シングルトンの取得
  // GetTransportType(): 現在のトランスポート層のタイプ（ENet または EOSP2P）を取得する関数
  UseEOSP2P = NetworkManager::GetInstance().GetTransportType() == ENetworkTransportType::EOSP2P;
  StatusMessage = "Select network role.";
  // Owner.GetWorld(): 所有者アクターが属するワールドインスタンスを取得する関数
  // GetGameMode(): 現在のワールドで稼働しているゲームモードを取得する関数
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    StatusMessage = std::string(gameMode->GetSceneName()) + " loaded.";
  }

  // Epic Online Services (EOS) が初期化されているかどうかを判定する関数
  // OnlineSessionManager::GetInstance(): EOSなどのオンラインセッション制御シングルトンの取得
  OnlineStatusMessage = OnlineSessionManager::GetInstance().IsEOSInitialized()
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
  // IsRunning(): ネットワーク接続が現在稼働しているかを取得する関数
  if (NetworkManager::GetInstance().IsRunning()) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Checkbox("Use EOS P2P", &RequestedEOSP2P)) {
    const ENetworkTransportType RequestedType =
        RequestedEOSP2P ? ENetworkTransportType::EOSP2P : ENetworkTransportType::ENet;
    // SetTransportType(): トランスポートのタイプ（ENet/EOSP2P）を設定する関数
    if (OnlineSessionManager::GetInstance().SetTransportType(RequestedType)) {
      UseEOSP2P = RequestedEOSP2P;
      StatusMessage =
          UseEOSP2P ? "EOS P2P selected. Login before starting or connecting." : "ENet selected.";
    }
  }
  // IsRunning(): ネットワーク接続が現在稼働しているかを取得する関数
  if (NetworkManager::GetInstance().IsRunning()) {
    ImGui::EndDisabled();
  }
  ImGui::TextDisabled(
      UseEOSP2P ? "Client target: host Product User ID" : "Client target: server IP"
  );

  ImGui::InputInt("Port", &Port);
  Port = std::clamp(Port, 1, 65535);

  if (ImGui::Button("Start Listen Server")) {
    // StartListenServer(): 自アクター（ホスト側）でリスンサーバーを起動して接続の待受を開始する関数
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
    // ConnectAsClient(): 指定のアドレスとポートへクライアントとして接続する関数
    if (Owner.ConnectAsClient(ServerAddress, static_cast<uint16_t>(Port))) {
      StatusMessage = "Connecting to server...";
    } else {
      StatusMessage = "Failed to connect to server.";
    }
  }

  ImGui::Separator();

  // GetGameMode(): 現在のワールドで稼働しているゲームモードを取得する関数
  ANetworkTestGameMode* gameMode =
      dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode());
  const char* sceneName = gameMode ? gameMode->GetSceneName() : "Unknown Scene";
  ImGui::Text("Current Scene: %s", sceneName);

  // サーバー権限を持つ（IsServer()）かつ、ゲームモードが存在する場合のみトラベル（レベル移行）処理を実行可能にする
  // IsServer(): 現在の実行環境がサーバー（オーソリティを持つ側）かどうかを判定する関数
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
  // EOS を統合管理するシングルトンマネージャーを取得する関数
  OnlineSessionManager& onlineSession = OnlineSessionManager::GetInstance();

  OnlineLobbyMaxMembers = std::clamp(OnlineLobbyMaxMembers, 1, 64);
  OnlineSearchMaxResults = std::clamp(OnlineSearchMaxResults, 1, 100);

  ImGui::Begin("EOS LAN Lobby Test");
  // EOS の初期化、ログイン状態、現在ロビー参加中かどうかを取得して UI に描画
  // IsEOSInitialized(): EOSが初期化されているかを判定する関数
  // IsLoggedIn(): ログイン状態かを判定する関数
  ImGui::Text("EOS initialized: %s", onlineSession.IsEOSInitialized() ? "true" : "false");
  ImGui::Text("Logged in: %s", onlineSession.IsLoggedIn() ? "true" : "false");
  if (onlineSession.IsLoggedIn()) {
    // ログイン中のユーザーID（ProductUserId）を取得する関数
    const std::string localUserId = onlineSession.GetLocalUserIdString();
    ImGui::TextWrapped("ProductUserId: %s", localUserId.c_str());
  }
  // IsInLobby(): ロビーに参加しているかを判定する関数
  ImGui::Text("In lobby: %s", onlineSession.IsInLobby() ? "true" : "false");
  if (onlineSession.IsInLobby()) {
    // 現在のロビー ID を取得する関数
    const std::string lobbyId = onlineSession.GetCurrentLobbyId();
    ImGui::TextWrapped("Current LobbyId: %s", lobbyId.c_str());
  }
  // 非同期の EOS API が現在進行中か（処理中か）を取得する関数
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
    // CanShowLogin(): ログイン処理が可能かを判定する関数
    if (onlineSession.CanShowLogin() && ImGui::Button("Login Device ID")) {
      LoginWithDeviceId();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (onlineSession.IsInLobby()) {
    // CanShowLeaveSession(): セッション退出処理が可能かを判定する関数
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
  // CanShowLobbyActions(): ロビー作成・破棄などの操作が可能かを判定する関数
  if (onlineSession.CanShowLobbyActions() && ImGui::Button("Create LAN Lobby")) {
    CreateLanLobby();
  }

  ImGui::InputInt("Search Max Results", &OnlineSearchMaxResults);
  // CanShowSearchLobbies(): ロビー検索が可能かを判定する関数
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

    // CanShowJoinLobby(): ロビーへの参加が可能かを判定する関数
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
  // IsListenServer(): 現在の実行環境が ListenServer 役割（ホスト＆クライアント）かどうかを判定する関数
  // IsClient(): 現在の実行環境が Client 役割（リモート接続）かどうかを判定する関数
  if (Owner.GetWorld()->IsListenServer()) {
    modeText = "ListenServer";
  } else if (Owner.GetWorld()->IsClient()) {
    modeText = "Client";
  }

  // ネットワーク処理の低レイヤーマネージャーを取得する関数
  NetworkManager& network = NetworkManager::GetInstance();
  ImGui::Text("Mode: %s", modeText);
  // ネットワーク接続が現在稼働しているかを取得する関数
  ImGui::Text("Network running: %s", network.IsRunning() ? "true" : "false");
  // ローカル環境自身のネットワーク接続IDを取得（未接続・単体起動時は無効値）する関数
  ImGui::Text("Local ConnectionId: %u", network.GetLocalConnectionId());

  bool bHostSpawned = false;
  if (auto gameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    // サーバーのホストプレイヤー（ホスト Pawn）がワールド上に生成済みか判定する関数
    bHostSpawned = gameMode->IsHostPlayerSpawned();
  }
  ImGui::Text("Host player spawned: %s", bHostSpawned ? "true" : "false");
  ImGui::End();
}

void FNetworkTestUI::LoginWithDeviceId() {
  // IsEOSInitialized(): Epic Online Services (EOS) が初期化されているかを判定する関数
  if (!OnlineSessionManager::GetInstance().IsEOSInitialized()) {
    OnlineStatusMessage = "EOS is not initialized.";
    return;
  }

  // IsLoggedIn(): ユーザーが現在ログイン状態であるかを判定する関数
  if (OnlineSessionManager::GetInstance().IsLoggedIn()) {
    OnlineStatusMessage = "Already logged in.";
    return;
  }

  OnlineStatusMessage = "EOS Device ID login requested.";
  // EOS Connect サービスのログイン API を実行（引数にユーザー名と成功可否を受け取るコールバックを渡す）
  // LoginWithDeviceId(): デバイスIDを用いて EOS Connect サービスへログインする非同期関数
  if (!OnlineSessionManager::GetInstance().LoginWithDeviceId(
          OnlineDisplayName, [this](bool bSuccess) {
            if (bSuccess) {
              OnlineStatusMessage = "EOS Device ID login succeeded.";
              // GetLocalUserIdString(): ログインに成功したユーザーの Product User ID を文字列として取得する関数
              const std::string localUserId =
                  OnlineSessionManager::GetInstance().GetLocalUserIdString();
              if (!localUserId.empty()) {
                OnlineStatusMessage += " ProductUserId: " + localUserId;
              }
            } else {
              OnlineStatusMessage = "EOS Device ID login failed. See Logs for details.";
            }
          }
      )) {
    OnlineStatusMessage = "EOS Device ID login request was rejected.";
  }
}

void FNetworkTestUI::CreateLanLobby() {
  // IsLoggedIn(): ユーザーが現在ログイン状態であるかを判定する関数
  if (!OnlineSessionManager::GetInstance().IsLoggedIn()) {
    OnlineStatusMessage = "Login before creating a lobby.";
    return;
  }

  // IsInLobby(): 現在ロビーセッションに参加中（またはホスト中）であるかを判定する関数
  if (OnlineSessionManager::GetInstance().IsInLobby()) {
    OnlineStatusMessage = "Already in a lobby. Leave it before creating another one.";
    return;
  }

  FCreateLobbyRequest request;
  request.MaxMembers = (std::max)(1, OnlineLobbyMaxMembers);
  request.bPublicAdvertised = true;
  request.Port = static_cast<uint16_t>(Port);
  // GetTransportType(): 現在のトランスポート層のタイプ（ENet または EOSP2P）を取得する関数
  if (NetworkManager::GetInstance().GetTransportType() == ENetworkTransportType::ENet) {
    // ENetだけがLobby AttributeとしてローカルIPアドレスを公開する。
    request.HostIPAddress = NetworkUtils::GetLocalIPAddress();
    if (request.HostIPAddress.empty()) {
      OnlineStatusMessage = "Local IP address was not found.";
      return;
    }
  }

  OnlineStatusMessage =
      "Create lobby requested. Transport=" +
      std::string(
          // GetTransportType(): 現在のトランスポート層のタイプを取得する関数
          NetworkManager::GetInstance().GetTransportType() == ENetworkTransportType::EOSP2P
              ? "EOSP2P"
              : "ENet"
      );
  // 新しい EOS ロビーの作成を非同期でリクエスト
  // CreateLobby(): 新しい EOS ロビーの作成を非同期でリクエストする関数
  if (!OnlineSessionManager::GetInstance().CreateLobby(
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
                  "Lobby host ready: " + lobbyInfo.LobbyId +
                  " Owner=" + lobbyInfo.OwnerProductUserId + " HostIP=" +
                  (lobbyInfo.HostIPAddress.empty() ? "<unused>" : lobbyInfo.HostIPAddress);
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
  // IsLoggedIn(): ユーザーが現在ログイン状態であるかを判定する関数
  if (!OnlineSessionManager::GetInstance().IsLoggedIn()) {
    OnlineStatusMessage = "Login before searching lobbies.";
    return;
  }

  FLobbySearchRequest request;
  request.MaxResults = (std::max)(1, OnlineSearchMaxResults);

  OnlineSearchResults.clear();
  SelectedOnlineLobbyIndex = -1;
  OnlineStatusMessage = "Search lobbies requested.";
  // 作成されているアクティブな EOS ロビーの検索を非同期で実行
  // SearchLobbies(): 作成されているアクティブな EOS ロビーの検索を非同期で実行する関数
  if (!OnlineSessionManager::GetInstance().SearchLobbies(
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
  // IsLoggedIn(): ユーザーが現在ログイン状態であるかを判定する関数
  if (!OnlineSessionManager::GetInstance().IsLoggedIn()) {
    OnlineStatusMessage = "Login before joining a lobby.";
    return;
  }

  // IsInLobby(): 現在ロビーセッションに参加中であるかを判定する関数
  if (OnlineSessionManager::GetInstance().IsInLobby()) {
    OnlineStatusMessage = "Already in a lobby. Leave it before joining another one.";
    return;
  }

  if (SelectedOnlineLobbyIndex < 0 ||
      SelectedOnlineLobbyIndex >= static_cast<int>(OnlineSearchResults.size())) {
    OnlineStatusMessage = "Select a lobby search result first.";
    return;
  }

  const FLobbyInfo lobbyInfo = OnlineSearchResults[SelectedOnlineLobbyIndex];
  // GetTransportType(): 現在のトランスポート層のタイプを取得する関数
  const bool bUseEOSP2P =
      NetworkManager::GetInstance().GetTransportType() == ENetworkTransportType::EOSP2P;
  if ((!bUseEOSP2P && lobbyInfo.HostIPAddress.empty()) ||
      (bUseEOSP2P && lobbyInfo.OwnerProductUserId.empty())) {
    OnlineStatusMessage = bUseEOSP2P ? "Selected lobby does not have an Owner Product User ID."
                                     : "Selected lobby does not have HostIP.";
    return;
  }

  OnlineStatusMessage = "Join lobby requested: " + lobbyInfo.LobbyId;
  // 選択した EOS ロビーへの参加およびホストへのネットワーク接続をリクエスト
  // JoinLobby(): 選択した EOS ロビーへの参加およびホストへのネットワーク接続をリクエストする関数
  if (!OnlineSessionManager::GetInstance().JoinLobby(
          lobbyInfo, static_cast<uint16_t>(Port), [this, lobbyInfo](bool bSuccess) {
            if (!bSuccess) {
              OnlineStatusMessage =
                  "Join lobby or transport connection failed. Search again if the result is stale.";
              return;
            }

            // GetTransportType(): 現在のトランスポート層 of タイプを取得する関数
            const std::string ConnectionTarget =
                NetworkManager::GetInstance().GetTransportType() == ENetworkTransportType::EOSP2P
                    ? lobbyInfo.OwnerProductUserId
                    : lobbyInfo.HostIPAddress;
            strncpy_s(ServerAddress, sizeof(ServerAddress), ConnectionTarget.c_str(), _TRUNCATE);
            // 接続成功後、ワールドのネットワークモードを Client に設定
            Owner.GetWorld()->SetNetMode(ENetMode::Client);
            StatusMessage = "Connecting to lobby host: " + ConnectionTarget;
            OnlineStatusMessage = "Joined lobby and connecting: " + lobbyInfo.LobbyId;
          }
      )) {
    OnlineStatusMessage = "Join lobby request was rejected.";
  }
}
void FNetworkTestUI::LeaveOnlineSession() {
  // IsInLobby(): 現在ロビーセッションに参加中であるかを判定する関数
  if (!OnlineSessionManager::GetInstance().IsInLobby()) {
    OnlineStatusMessage = "Not in a lobby.";
    return;
  }

  // GetCurrentLobbyId(): 参加中のロビーのIDを取得する関数
  const std::string lobbyId = OnlineSessionManager::GetInstance().GetCurrentLobbyId();
  OnlineStatusMessage = "Leave session requested: " + lobbyId;
  // 現在参加しているマルチプレイロビーから退出
  // LeaveSession(): 現在参加しているマルチプレイロビーから退出する非同期関数
  if (!OnlineSessionManager::GetInstance().LeaveSession([this, lobbyId](bool bSuccess) {
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
