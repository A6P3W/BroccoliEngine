#pragma once
#include "BroccoliEngineAPI.h"

#include "EOSTypes.h"
#include "NetworkTypes.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class BROCCOLI_ENGINE_API OnlineSessionManager {
 public:
  static OnlineSessionManager& Get();

  bool LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  bool CreateLobby(const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete);
  bool UpdateCurrentLobbyAttributes(
      const std::vector<FLobbyAttribute>& Attributes,
      std::function<void(bool)> OnComplete
  );
  bool SearchLobbies(
      const FLobbySearchRequest& Request,
      std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
  );
  bool FetchLobbyInfoById(
      const std::string& LobbyId,
      std::function<void(bool, const FLobbyInfo&)> OnComplete
  );
  bool JoinLobby(const FLobbyInfo& LobbyInfo, uint16_t Port, std::function<void(bool)> OnComplete);
  bool LeaveSession(std::function<void(bool)> OnComplete);
  bool LeaveLobby(std::function<void(bool)> OnComplete);
  void Shutdown();

  bool IsEOSInitialized() const;
  bool IsLoggedIn() const;
  bool IsInLobby() const;
  bool IsOperationPending() const;

  bool CanShowLogin() const;
  bool CanShowLobbyActions() const;
  bool CanShowSearchLobbies() const;
  bool CanShowJoinLobby() const;
  bool CanShowLeaveSession() const;
  bool CanShowLeaveLobby() const;

  std::string GetLocalUserIdString() const;
  std::string GetCurrentLobbyId() const;

 private:
  OnlineSessionManager();
  ~OnlineSessionManager();

  OnlineSessionManager(const OnlineSessionManager&) = delete;
  OnlineSessionManager& operator=(const OnlineSessionManager&) = delete;

 private:
  bool BeginOperation();
  void EndOperation();
  void NotifySessionDisconnected(ELobbyDisconnectReason Reason);
  void HandleLobbyDisconnected(ELobbyDisconnectReason Reason);
  void HandleNetworkDisconnected(FNetworkConnectionId ConnectionId);
  void HandleAuthLost(EAuthLossReason Reason);

 private:
  bool bOperationPending = false;
  bool bIsLeavingSession = false;
  size_t NetworkDisconnectedCallbackHandle = 0;
};
