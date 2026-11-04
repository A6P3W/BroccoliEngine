#ifdef BROCCOLI_WITH_EOSPLUGIN
#pragma once
#include <EOSPlugin/EOSPlugin.h>
using namespace BEPlugins::EOS;

#include <string>
#include <vector>

class ANetworkTestPawn;

// ネットワーク接続や EOS ロビー操作を画面上に ImGui を使用して描画するデバッグ UI クラス
class FNetworkTestUI {
 public:
  explicit FNetworkTestUI(ANetworkTestPawn& InOwner);

  // ステータス表示の初期化処理
  void InitializeStatus();
  // デバッグウィンドウを描画するメイン処理
  void Draw();
  void SetStatusMessage(const std::string& Message);

 private:
  void DrawConnectionWindow();
  void DrawOnlineWindow();
  void DrawStatusWindow();
  void DownloadLevel();
  void ServerTravelByPath();

  // EOS Connect サービスへデバイス ID を用いてログインする関数
  void LoginWithDeviceId();
  // サーバーとして オンラインロビーを新規作成する関数
  void HostOnlineLobby();
  // アクティブな EOS ロビーを検索する関数
  void SearchOnlineLobbies();
  // 選択された EOS ロビーにクライアントとして参加する関数
  void JoinSelectedLobby();
  // 現在の オンラインロビーから退出する関数
  void LeaveOnlineLobby();

  ANetworkTestPawn& Owner;

  bool UseEOSP2P = false;
  int Port = 7777;
  std::string StatusMessage;
  char DownloadLevelFileName[260] = "Stage01.BLevel";

  char OnlineDisplayName[32] = "BroccoliPlayer";
  int OnlineLobbyMaxMembers = 4;
  int OnlineSearchMaxResults = 10;
  std::vector<FLobbyInfo> OnlineSearchResults;
  int SelectedOnlineLobbyIndex = -1;
  std::string OnlineStatusMessage;
};

#else
#pragma once
#include <string>
class ANetworkTestPawn;
class FNetworkTestUI {
 public:
  explicit FNetworkTestUI(ANetworkTestPawn& InOwner) : Owner(InOwner) {}
  void InitializeStatus();
  void Draw();
  void SetStatusMessage(const std::string& Message) { StatusMessage = Message; }

 private:
  ANetworkTestPawn& Owner;
  std::string StatusMessage;
  int Port = 7777;
  char Address[256] = "127.0.0.1";
};
#endif
