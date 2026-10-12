#ifdef BROCCOLI_WITH_EOSPLUGIN
#include "NetworkTestUI.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "FileDialog.h"
#include "NetworkManager.h"
#include "NetworkTestGameMode.h"
#include "NetworkTestPawn.h"
#include "World.h"

FNetworkTestUI::FNetworkTestUI(ANetworkTestPawn& InOwner) : Owner(InOwner) {}

void FNetworkTestUI::InitializeStatus() {
  // NetworkManager::GetInstance(): ネットワーク制御シングルトンの取得
  // GetTransportName(): 現在のトランスポート層のタイプ（ENet または EOSP2P）を取得する関数
  UseEOSP2P = NetworkManager::GetInstance().GetTransportName() == "EOSP2P";
  StatusMessage = "Select network role.";
  // Owner.GetWorld(): 所有者アクターが属するワールドインスタンスを取得する関数
  // GetGameMode(): 現在のワールドで稼働しているゲームモードを取得する関数
  if (auto GameMode = dynamic_cast<ANetworkTestGameMode*>(Owner.GetWorld()->GetGameMode())) {
    StatusMessage = std::string(GameMode->GetSceneName()) + " loaded.";
  }

  // Epic Online Services (EOS) が初期化されているかどうかを判定する関数
  // Plugin Public API から EOS の初期化状態を取得
  OnlineStatusMessage = BEPlugins::EOS::IsEOSInitialized() ? "EOS initialized. Login to use lobby."
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

  bool RequestedEOSP2P = UseEOSP2P;
  if (BEPlugins::EOS::IsOperationPending() || BEPlugins::EOS::IsInLobby() ||
      NetworkManager::GetInstance().IsRunning()) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Checkbox("Use EOS P2P", &RequestedEOSP2P)) {
    const std::string RequestedType = RequestedEOSP2P ? "EOSP2P" : "ENet";
    if (BEPlugins::EOS::ConfigureTransport(RequestedType)) {
      UseEOSP2P = RequestedEOSP2P;
      StatusMessage = UseEOSP2P ? "EOS P2P transport selected." : "ENet transport selected.";
    } else {
      StatusMessage = "Transport change was rejected. Leave online play before changing it.";
    }
  }
  if (BEPlugins::EOS::IsOperationPending() || BEPlugins::EOS::IsInLobby() ||
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

  OnlineLobbyMaxMembers = std::clamp(OnlineLobbyMaxMembers, 1, 64);
  OnlineSearchMaxResults = std::clamp(OnlineSearchMaxResults, 1, 100);

  ImGui::Begin("Online Play");
  // EOS の初期化、ログイン状態、現在ロビー参加中かどうかを取得して UI に描画
  // IsEOSInitialized(): EOSが初期化されているかを判定する関数
  // IsLoggedIn(): ログイン状態かを判定する関数
  ImGui::Text("EOS initialized: %s", BEPlugins::EOS::IsEOSInitialized() ? "true" : "false");
  ImGui::Text("Logged in: %s", BEPlugins::EOS::IsLoggedIn() ? "true" : "false");
  if (BEPlugins::EOS::IsLoggedIn()) {
    // ログイン中のユーザーID（ProductUserId）を取得する関数
    const std::string LocalUserId = BEPlugins::EOS::GetLocalUserIdString();
    ImGui::TextWrapped("ProductUserId: %s", LocalUserId.c_str());
  }
  // IsInLobby(): ロビーに参加しているかを判定する関数
  ImGui::Text("In lobby: %s", BEPlugins::EOS::IsInLobby() ? "true" : "false");
  if (BEPlugins::EOS::IsInLobby()) {
    // 現在のロビー ID を取得する関数
    const std::string LobbyId = BEPlugins::EOS::GetCurrentLobbyId();
    ImGui::TextWrapped("Current LobbyId: %s", LobbyId.c_str());
  }
  // 非同期の EOS API が現在進行中か（処理中か）を取得する関数
  ImGui::Text("Operation pending: %s", BEPlugins::EOS::IsOperationPending() ? "true" : "false");
  ImGui::Separator();

  if (!BEPlugins::EOS::IsEOSInitialized()) {
    ImGui::TextDisabled("EOS is not initialized.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (!BEPlugins::EOS::IsLoggedIn()) {
    ImGui::InputText("Display Name", OnlineDisplayName, sizeof(OnlineDisplayName));
    // CanLogin(): ログイン処理が可能かを判定する関数
    if (BEPlugins::EOS::CanLogin() && ImGui::Button("Login Device ID")) {
      LoginWithDeviceId();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (BEPlugins::EOS::IsInLobby()) {
    // CanLeaveLobby(): セッション退出処理が可能かを判定する関数
    if (BEPlugins::EOS::CanLeaveLobby() && ImGui::Button("Leave Lobby")) {
      LeaveOnlineLobby();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  if (BEPlugins::EOS::IsOperationPending()) {
    ImGui::TextDisabled("Online play operation is pending.");
    ImGui::Separator();
    ImGui::TextWrapped("%s", OnlineStatusMessage.c_str());
    ImGui::End();
    return;
  }

  ImGui::InputInt("Max Members", &OnlineLobbyMaxMembers);
  // CanHostLobby(): ロビー作成・破棄などの操作が可能かを判定する関数
  if (BEPlugins::EOS::CanHostLobby() && ImGui::Button("Host Lobby")) {
    HostOnlineLobby();
  }

  ImGui::InputInt("Search Max Results", &OnlineSearchMaxResults);
  // CanSearchLobbies(): ロビー検索が可能かを判定する関数
  if (BEPlugins::EOS::CanSearchLobbies() && ImGui::Button("Search Lobbies")) {
    SearchOnlineLobbies();
  }

  if (!OnlineSearchResults.empty()) {
    ImGui::BeginChild("OnlineLobbySearchResults", ImVec2(0.0f, 120.0f), true);
    for (int Index = 0; Index < static_cast<int>(OnlineSearchResults.size()); ++Index) {
      const FLobbyInfo& lobbyInfo = OnlineSearchResults[Index];
      std::string Label =
          std::to_string(Index + 1) + ": " + lobbyInfo.LobbyId + " (" +
          std::to_string(lobbyInfo.CurrentMembers) + "/" + std::to_string(lobbyInfo.MaxMembers) +
          ") Host=" + (lobbyInfo.HostIPAddress.empty() ? "<none>" : lobbyInfo.HostIPAddress);
      if (ImGui::Selectable(Label.c_str(), SelectedOnlineLobbyIndex == Index)) {
        SelectedOnlineLobbyIndex = Index;
      }
    }
    ImGui::EndChild();

    // CanJoinLobby(): ロビーへの参加が可能かを判定する関数
    if (BEPlugins::EOS::CanJoinLobby() && ImGui::Button("Join Selected Lobby")) {
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

  World* OwnerWorld = Owner.GetWorld();
  const char* ModeText = "Standalone";
  // ワールドが ListenServer（サーバー＆クライアント）か、Client かを判定して表記を分ける
  // IsListenServer(): 現在の実行環境が ListenServer 役割（ホスト＆クライアント）かどうかを判定する関数
  // IsClient(): 現在の実行環境が Client 役割（リモート接続）かどうかを判定する関数
  if (OwnerWorld->IsListenServer()) {
    ModeText = "ListenServer";
  } else if (OwnerWorld->IsClient()) {
    ModeText = "Client";
  }

  // ネットワーク処理の低レイヤーマネージャーを取得する関数
  NetworkManager& Network = NetworkManager::GetInstance();
  ImGui::Text("Mode: %s", ModeText);
  // ネットワーク接続が現在稼働しているかを取得する関数
  ImGui::Text("Network running: %s", Network.IsRunning() ? "true" : "false");
  // ローカル環境自身のネットワーク接続IDを取得（未接続・単体起動時は無効値）する関数
  ImGui::Text("Local ConnectionId: %u", Network.GetLocalConnectionId());

  bool HostSpawned = false;
  ANetworkTestGameMode* GameMode = dynamic_cast<ANetworkTestGameMode*>(OwnerWorld->GetGameMode());
  if (GameMode) {
    // サーバーのホストプレイヤー（ホスト Pawn）がワールド上に生成済みか判定する関数
    HostSpawned = GameMode->IsHostPlayerSpawned();
  }
  ImGui::Text("Host player spawned: %s", HostSpawned ? "true" : "false");

  ImGui::Separator();
  ImGui::InputText("Download file name", DownloadLevelFileName, sizeof(DownloadLevelFileName));
  const bool DownloadPending = BEPlugins::EOS::IsDownloadPending();
  if (DownloadPending) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("DownloadLevel")) {
    DownloadLevel();
  }
  if (DownloadPending) {
    ImGui::EndDisabled();
    ImGui::TextDisabled("A level download is in progress.");
  }

  if (GameMode) {
    const char* TargetLevelPath = GameMode->GetTravelTargetLevelPath();
    ImGui::Separator();
    ImGui::TextWrapped("Server travel target: %s", TargetLevelPath);

    const bool CanServerTravel = OwnerWorld->IsListenServer() && !DownloadPending;
    if (!CanServerTravel) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button(GameMode->GetTravelButtonText())) {
      const bool TravelQueued = OwnerWorld->ServerTravel(std::string(TargetLevelPath));
      StatusMessage = TravelQueued ? std::string("Server travel queued: ") + TargetLevelPath
                                   : std::string("Server travel failed: ") + TargetLevelPath;
    }
    if (!CanServerTravel) {
      ImGui::EndDisabled();
      if (DownloadPending) {
        ImGui::TextDisabled("Wait for the level download to finish before server travel.");
      } else {
        ImGui::TextDisabled("Server travel is available only on the listen server.");
      }
    }

    if (!CanServerTravel) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("ServerTravelByPath")) {
      ServerTravelByPath();
    }
    if (!CanServerTravel) {
      ImGui::EndDisabled();
    }
  }
  ImGui::End();
}

void FNetworkTestUI::DownloadLevel() {
  StatusMessage = std::string("Level download requested: ") + DownloadLevelFileName;
  BEPlugins::EOS::Download(
      DownloadLevelFileName, [this](const FTitleStorageDownloadResult& Result) {
        StatusMessage = Result.Message;
        if (Result.Success) {
          StatusMessage += " Saved to: " + Result.LocalFilePath;
        }
      }
  );
}

void FNetworkTestUI::ServerTravelByPath() {
  const std::string LevelPath =
      FileDialog::OpenFile("Broccoli Level Files (*.BLevel)\0*.BLevel\0All Files (*.*)\0*.*\0");
  if (LevelPath.empty()) {
    return;
  }

  World* OwnerWorld = Owner.GetWorld();
  const bool TravelQueued = OwnerWorld && OwnerWorld->ServerTravel(LevelPath);
  StatusMessage =
      TravelQueued ? "Server travel queued: " + LevelPath : "Server travel failed: " + LevelPath;
}

void FNetworkTestUI::LoginWithDeviceId() {
  OnlineStatusMessage = "EOS Device ID login requested.";
  BEPlugins::EOS::Login(OnlineDisplayName, [this](const FOnlinePlayResult& Result) {
    OnlineStatusMessage = Result.Message;
    if (Result.Success) {
      const std::string LocalUserId = BEPlugins::EOS::GetLocalUserIdString();
      if (!LocalUserId.empty()) {
        OnlineStatusMessage += " ProductUserId: " + LocalUserId;
      }
    }
  });
}

void FNetworkTestUI::HostOnlineLobby() {
  FHostLobbyRequest Request;
  Request.MaxMembers = (std::max)(1, OnlineLobbyMaxMembers);
  Request.PublicAdvertised = true;
  Request.Port = static_cast<uint16_t>(Port);

  OnlineStatusMessage = "Host lobby requested.";
  BEPlugins::EOS::HostLobby(Request, [this](const FOnlinePlayResult& Result) {
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

  BEPlugins::EOS::SearchLobbies(
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
  BEPlugins::EOS::JoinLobby(
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
  const std::string LobbyId = BEPlugins::EOS::GetCurrentLobbyId();
  OnlineStatusMessage = "Leave lobby requested: " + LobbyId;
  BEPlugins::EOS::LeaveLobby([this, LobbyId](const FOnlinePlayResult& Result) {
    OnlineStatusMessage = Result.Message;
    if (Result.Success) {
      StatusMessage = "Returned to standalone play.";
      OnlineStatusMessage += " LobbyId=" + LobbyId;
    }
  });
}
#else
#include <imgui.h>

#include "NetworkManager.h"
#include "NetworkTestPawn.h"
#include "NetworkTestUI.h"
#include "World.h"

void FNetworkTestUI::InitializeStatus() { StatusMessage = "ENet ready."; }
void FNetworkTestUI::Draw() {
  auto& Network = NetworkManager::GetInstance();
  ImGui::Begin("Network Transport");
  ImGui::Text("Transport: %s", Network.GetTransportName().c_str());
  ImGui::InputText("Address", Address, sizeof(Address));
  ImGui::InputInt("Port", &Port);
  if (ImGui::Button("Host") && Port > 0 && Port <= 65535) {
    if (Network.StartServer(static_cast<uint16_t>(Port)))
      Owner.GetWorld()->SetNetMode(ENetMode::ListenServer);
  }
  if (ImGui::Button("Connect") && Port > 0 && Port <= 65535) {
    if (Network.ConnectToServer(Address, static_cast<uint16_t>(Port)))
      Owner.GetWorld()->SetNetMode(ENetMode::Client);
  }
  if (ImGui::Button("Stop")) {
    Network.Stop();
    Owner.GetWorld()->SetNetMode(ENetMode::Standalone);
  }
  ImGui::TextWrapped("%s", StatusMessage.c_str());
  ImGui::End();
}
#endif
