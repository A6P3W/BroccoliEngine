#pragma once

#include "EOSTypes.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class OnlineSessionManager {
 public:
  static OnlineSessionManager& Get();

  bool LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  bool CreateLobby(const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete);
  bool SearchLobbies(
      const FLobbySearchRequest& Request,
      std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
  );
  bool JoinLobby(const FLobbyInfo& LobbyInfo, uint16_t Port, std::function<void(bool)> OnComplete);
  bool LeaveLobby(std::function<void(bool)> OnComplete);

  bool IsEOSInitialized() const;
  bool IsLoggedIn() const;
  bool IsInLobby() const;
  bool IsOperationPending() const;

  bool CanShowLogin() const;
  bool CanShowLobbyActions() const;
  bool CanShowSearchLobbies() const;
  bool CanShowJoinLobby() const;
  bool CanShowLeaveLobby() const;

  std::string GetLocalUserIdString() const;
  std::string GetCurrentLobbyId() const;

 private:
  OnlineSessionManager();
  ~OnlineSessionManager() = default;

  OnlineSessionManager(const OnlineSessionManager&) = delete;
  OnlineSessionManager& operator=(const OnlineSessionManager&) = delete;

 private:
  bool BeginOperation();
  void EndOperation();
  void HandleLobbyDisconnected(ELobbyDisconnectReason Reason);
  void HandleAuthLost(EAuthLossReason Reason);

 private:
  bool bOperationPending = false;
};
