#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "EOSTypes.h"
#include "NetMode.h"
#include "NetworkTransport.h"
#include "NetworkTypes.h"

class EOSOnlinePlayManager {
 public:
  using ResultCallback = std::function<void(const FOnlinePlayResult&)>;
  using SearchCallback =
      std::function<void(const FOnlinePlayResult&, const std::vector<FLobbyInfo>&)>;
  static EOSOnlinePlayManager& GetInstance();
  bool ConfigureTransport(std::string_view Name);
  const std::string& GetTransportName() const;
  bool Login(const char* DisplayName, ResultCallback OnComplete);
  bool HostLobby(const FHostLobbyRequest& Request, ResultCallback OnComplete);
  bool SearchLobbies(const FLobbySearchRequest& Request, SearchCallback OnComplete);
  bool JoinLobby(const FLobbyInfo& LobbyInfo, uint16_t Port, ResultCallback OnComplete);
  bool LeaveLobby(ResultCallback OnComplete);
  void Shutdown();
  bool IsEOSInitialized() const;
  bool IsLoggedIn() const;
  bool IsInLobby() const;
  bool IsOperationPending() const;
  EOnlinePlayState GetState() const;
  bool CanLogin() const;
  bool CanHostLobby() const;
  bool CanSearchLobbies() const;
  bool CanJoinLobby() const;
  bool CanLeaveLobby() const;
  std::string GetLocalUserIdString() const;
  std::string GetCurrentLobbyId() const;

 private:
  EOSOnlinePlayManager();
  ~EOSOnlinePlayManager();
  EOSOnlinePlayManager(const EOSOnlinePlayManager&) = delete;
  EOSOnlinePlayManager& operator=(const EOSOnlinePlayManager&) = delete;
  bool BeginOperation(EOnlinePlayState NewState);
  void EndOperation(EOnlinePlayState NewState = EOnlinePlayState::Idle);
  FOnlinePlayResult MakeResult(
      bool Success,
      EOnlinePlayError Error,
      std::string Message,
      const FLobbyInfo& LobbyInfo = {},
      bool RollbackSucceeded = true
  ) const;
  void CompleteResult(ResultCallback& OnComplete, const FOnlinePlayResult& Result) const;
  bool ApplyNetMode(ENetMode NetMode);
  void RollbackLobby(ResultCallback OnComplete, EOnlinePlayError Error, std::string Message);
  void HandleHostLobbyCreated(
      const FCreateLobbyRequest& RoutingRequest,
      std::string TransportName,
      ResultCallback OnComplete,
      bool Success,
      const FLobbyInfo& LobbyInfo
  );
  void NotifySessionDisconnected(ELobbyDisconnectReason Reason);
  void BeginSessionEnd(
      ESessionDisconnectReason Reason,
      bool LeaveEOSLobby,
      bool Notify,
      ResultCallback OnComplete = nullptr
  );
  void FinalizeSessionEnd(
      ESessionDisconnectReason Reason, bool Success, bool Notify, ResultCallback OnComplete
  );
  void HandleLobbyDisconnected(ELobbyDisconnectReason Reason);
  void HandleNetworkDisconnected(
      FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason
  );
  void HandleAuthLost(EAuthLossReason Reason);
  EOnlinePlayState State = EOnlinePlayState::Idle;
  bool OperationPending = false;
  bool IsLeavingSession = false;
  bool IsEndingSession = false;
  bool SessionDisconnectNotified = false;
  size_t NetworkDisconnectedCallbackHandle = 0;
};
