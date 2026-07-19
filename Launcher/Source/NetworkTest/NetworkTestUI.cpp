#include "NetworkTestUI.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "NetworkManager.h"
#include "NetworkTestGameMode.h"
#include "NetworkTestPawn.h"
#include "OnlinePlayManager.h"
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
  // OnlinePlayManager::GetInstance(): EOSなどのオンラインセッション制御シングルトンの取得
  OnlineStatusMessage = OnlinePlayManager::GetInstance().IsEOSInitialized()
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
  ImGui::Begin("Network Transport");
  ImGui::TextUnformatted("Online Play Transport Settings");
  ImGui::Separator();

  OnlinePlayManager& OnlinePlay = OnlinePlayManager::GetInstance();
  bool RequestedEOSP2P = UseEOSP2P;
  if (OnlinePlay.IsOperationPending() || OnlinePlay.IsInLobby() ||
      NetworkManager::GetInstance().IsRunning()) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Checkbox("Use EOS P2P", &RequestedEOSP2P)) {
    const ENetworkTransportType RequestedType =
        RequestedEOSP2P ? ENetworkTransportType::EOSP2P : ENetworkTransportType::ENet;
    if (OnlinePlay.ConfigureTransport(RequestedType)) {
      UseEOSP2P = RequestedEOSP2P;
      StatusMessage = UseEOSP2P ? "EOS P2P transport selected." : "ENet transport selected.";
    } else {
      StatusMessage = "Transport change was rejected. Leave online play before changing it.";
    }
  }
  if (OnlinePlay.IsOperationPending() || OnlinePlay.IsInLobby() ||
      NetworkManager::GetInstance().IsRunning()) {
    ImGui::EndDisabled();
  }

  ImGui::InputInt("Game Port", &Port);
  Port = std::clamp(Port, 1, 65535);
  ImGui::TextDisabled("Hosting and joining are controlled from the Online Play window.");
  ImGui::Separator();
  ImGui::TextWrapped("%s", StatusMessage.c_str());
  ImGui::End();
}
void FNetworkTestUI::DrawOnlineWindow() {
  // EOS を統合管理するシングルトンマネージャーを取得する関数
  OnlinePlayManager& OnlinePlay = OnlinePlayManager::GetInstance();

  OnlineLobbyMaxMembers = std::clamp(OnlineLobbyMaxMembers, 1, 64);
  OnlineSearchMaxResults = std::clamp(OnlineSearchMaxResults, 1, 100);

  ImGui::Begin("Online Play");
  // EOS の初期化、ログイン状態、現在ロビー参加中かどうかを取得して UI に描画
  // IsEOSInitialized(): EOSが初期化されているかを判定する関数
  // IsLoggedIn(): ログイン状態かを判定する関数
  ImGui::Text("EOS initialized: %s", OnlinePlay.IsEOSInitialized() ? "true" : "false");
  ImGui::Text("Logged in: %s", OnlinePlay.IsLoggedIn() ? "true" : "false");
  if (OnlinePlay.IsLoggedIn()) {
    // ログイン中のユーザーID（ProductUserId）を取得する関数
    const std::string localUserId = OnlinePlay.GetLocalUserIdString();
    ImGui::TextWrapped("ProductUserId: %s", localUserId.c_str());
  }
  // IsInLobby(): ロビーに参加しているかを判定する関数
  ImGui::Text("In lobby: %s", OnlinePlay.IsInLobby() ? "true" : "false");
  if (OnlinePlay.IsInLobby()) {
    // 現在のロビー ID を取得する関数
    const std::string lobbyId = OnlinePlay.GetCurrentLobbyId();
    ImGui::TextWrapped("Current LobbyId: %s", lobbyId.c_str());
  }
  // 非同期の EOS API が現在進行中か（処理中か）を取得する関数
  ImGui::Text("Operation pending: %s", OnlinePlay.IsOperationPending() ? "true" : "false");
  ImGui::Separator();

  if (!OnlinePlay.IsEOSInitialized()) {
    ImGui::TextDisabled("EOS is not initialized.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (!OnlinePlay.IsLoggedIn()) {
    ImGui::InputText("Display Name", OnlineDisplayName, sizeof(OnlineDisplayName));
    // CanLogin(): ログイン処理が可能かを判定する関数
    if (OnlinePlay.CanLogin() && ImGui::Button("Login Device ID")) {
      LoginWithDeviceId();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (OnlinePlay.IsInLobby()) {
    // CanLeaveLobby(): セッション退出処理が可能かを判定する関数
    if (OnlinePlay.CanLeaveLobby() && ImGui::Button("Leave Lobby")) {
      LeaveOnlineLobby();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (OnlinePlay.IsOperationPending()) {
    ImGui::TextDisabled("Online play operation is pending.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  ImGui::InputInt("Max Members", &OnlineLobbyMaxMembers);
  // CanHostLobby(): ロビー作成・破棄などの操作が可能かを判定する関数
  if (OnlinePlay.CanHostLobby() && ImGui::Button("Host Lobby")) {
    HostOnlineLobby();
  }

  ImGui::InputInt("Search Max Results", &OnlineSearchMaxResults);
  // CanSearchLobbies(): ロビー検索が可能かを判定する関数
  if (OnlinePlay.CanSearchLobbies() && ImGui::Button("Search Lobbies")) {
    SearchOnlineLobbies();
  }

  if (!OnlineSearchResults.empty()) {
    ImGui::BeginChild("OnlineLobbySearchResults", ImVec2(0.0f, 120.0f), true);
    for (int index = 0; index < static_cast<int>(OnlineSearchResults.size()); ++index) {
      const FLobbyInfo& lobbyInfo = OnlineSearchResults[index];
      std::string label =
          std::to_string(index + 1) + ": " + lobbyInfo.LobbyId + " (" +
          std::to_string(lobbyInfo.CurrentMembers) + "/" + std::to_string(lobbyInfo.MaxMembers) +
          ") Host=" + (lobbyInfo.HostIPAddress.empty() ? "<none>" : lobbyInfo.HostIPAddress);
      if (ImGui::Selectable(label.c_str(), SelectedOnlineLobbyIndex == index)) {
        SelectedOnlineLobbyIndex = index;
      }
    }
    ImGui::EndChild();

    // CanJoinLobby(): ロビーへの参加が可能かを判定する関数
    if (OnlinePlay.CanJoinLobby() && ImGui::Button("Join Selected Lobby")) {
      JoinSelectedLobby();
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
  OnlineStatusMessage = "EOS Device ID login requested.";
  OnlinePlayManager::GetInstance().Login(
      OnlineDisplayName, [this](const FOnlinePlayResult& Result) {
        OnlineStatusMessage = Result.Message;
        if (Result.Success) {
          const std::string LocalUserId = OnlinePlayManager::GetInstance().GetLocalUserIdString();
          if (!LocalUserId.empty()) {
            OnlineStatusMessage += " ProductUserId: " + LocalUserId;
          }
        }
      }
  );
}

void FNetworkTestUI::HostOnlineLobby() {
  FHostLobbyRequest Request;
  Request.MaxMembers = (std::max)(1, OnlineLobbyMaxMembers);
  Request.PublicAdvertised = true;
  Request.Port = static_cast<uint16_t>(Port);

  OnlineStatusMessage = "Host lobby requested.";
  OnlinePlayManager::GetInstance().HostLobby(Request, [this](const FOnlinePlayResult& Result) {
    OnlineStatusMessage = Result.Message;
    if (!Result.Success) {
      return;
    }

    OnlineSearchResults.clear();
    SelectedOnlineLobbyIndex = -1;
    StatusMessage = "Online lobby host is ready.";
    OnlineStatusMessage += " LobbyId=" + Result.LobbyInfo.LobbyId;
  });
}

void FNetworkTestUI::SearchOnlineLobbies() {
  FLobbySearchRequest Request;
  Request.MaxResults = (std::max)(1, OnlineSearchMaxResults);
  OnlineSearchResults.clear();
  SelectedOnlineLobbyIndex = -1;
  OnlineStatusMessage = "Lobby search requested.";

  OnlinePlayManager::GetInstance().SearchLobbies(
      Request, [this](const FOnlinePlayResult& Result, const std::vector<FLobbyInfo>& Results) {
        OnlineStatusMessage = Result.Message;
        if (!Result.Success) {
          return;
        }

        OnlineSearchResults = Results;
        SelectedOnlineLobbyIndex = OnlineSearchResults.empty() ? -1 : 0;
        OnlineStatusMessage += " Found: " + std::to_string(OnlineSearchResults.size());
      }
  );
}

void FNetworkTestUI::JoinSelectedLobby() {
  if (SelectedOnlineLobbyIndex < 0 ||
      SelectedOnlineLobbyIndex >= static_cast<int>(OnlineSearchResults.size())) {
    OnlineStatusMessage = "Select a lobby search result first.";
    return;
  }

  const FLobbyInfo LobbyInfo = OnlineSearchResults[SelectedOnlineLobbyIndex];
  OnlineStatusMessage = "Join lobby requested: " + LobbyInfo.LobbyId;
  OnlinePlayManager::GetInstance().JoinLobby(
      LobbyInfo, static_cast<uint16_t>(Port), [this](const FOnlinePlayResult& Result) {
        OnlineStatusMessage = Result.Message;
        if (Result.Success) {
          StatusMessage = "Connected to online lobby host.";
          OnlineStatusMessage += " LobbyId=" + Result.LobbyInfo.LobbyId;
        }
      }
  );
}

void FNetworkTestUI::LeaveOnlineLobby() {
  const std::string LobbyId = OnlinePlayManager::GetInstance().GetCurrentLobbyId();
  OnlineStatusMessage = "Leave lobby requested: " + LobbyId;
  OnlinePlayManager::GetInstance().LeaveLobby([this, LobbyId](const FOnlinePlayResult& Result) {
    OnlineStatusMessage = Result.Message;
    if (Result.Success) {
      StatusMessage = "Returned to standalone play.";
      OnlineStatusMessage += " LobbyId=" + LobbyId;
    }
  });
}