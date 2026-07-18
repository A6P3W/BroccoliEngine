#pragma once

#include <string>
#include <vector>

#include "EOSTypes.h"

class ANetworkTestPawn;

class FNetworkTestUI {
 public:
  explicit FNetworkTestUI(ANetworkTestPawn& InOwner);

  void InitializeStatus();
  void Draw();
  void SetStatusMessage(const std::string& Message);

 private:
  void DrawConnectionWindow();
  void DrawOnlineWindow();
  void DrawStatusWindow();

  void LoginWithDeviceId();
  void CreateLanLobby();
  void SearchOnlineLobbies();
  void JoinSelectedLanLobby();
  void LeaveOnlineSession();

  ANetworkTestPawn& Owner;

  char ServerAddress[64] = "127.0.0.1";
  int Port = 7777;
  std::string StatusMessage;

  char OnlineDisplayName[32] = "BroccoliPlayer";
  int OnlineLobbyMaxMembers = 4;
  int OnlineSearchMaxResults = 10;
  std::vector<FLobbyInfo> OnlineSearchResults;
  int SelectedOnlineLobbyIndex = -1;
  std::string OnlineStatusMessage;
};
