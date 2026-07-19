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

enum class EOnlinePlayState : uint8_t {
  Idle,
  LoggingIn,
  HostingLobby,
  SearchingLobbies,
  JoiningLobby,
  Hosting,
  Connected,
  Leaving
};
enum class EOnlinePlayError : uint8_t {
  None,
  EOSNotInitialized,
  NotLoggedIn,
  AlreadyLoggedIn,
  AlreadyInLobby,
  OperationInProgress,
  InvalidRequest,
  LocalAddressNotFound,
  LoginFailed,
  LobbyCreationFailed,
  LobbySearchFailed,
  LobbyJoinFailed,
  TransportStartFailed,
  TransportConnectFailed,
  NetModeApplyFailed,
  LobbyLeaveFailed,
  RollbackFailed,
  Unknown
};

struct FHostLobbyRequest {
  int MaxMembers = 4;
  bool PublicAdvertised = true;
  std::string BucketId = "BroccoliNetworkTest";
  uint16_t Port = 7777;
  std::vector<FLobbyAttribute> Attributes;
};

struct FOnlinePlayResult {
  bool Success = false;
  EOnlinePlayError Error = EOnlinePlayError::Unknown;
  std::string Message;
  FLobbyInfo LobbyInfo;
  ENetworkTransportType TransportType = ENetworkTransportType::ENet;
  bool RollbackSucceeded = true;
};

class BROCCOLI_ENGINE_API OnlinePlayManager {
 public:
  using ResultCallback = std::function<void(const FOnlinePlayResult&)>;
  using SearchCallback =
      std::function<void(const FOnlinePlayResult&, const std::vector<FLobbyInfo>&)>;
  static OnlinePlayManager& GetInstance();
  bool ConfigureTransport(ENetworkTransportType Type);
  ENetworkTransportType GetTransportType() const;
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
  OnlinePlayManager();
  ~OnlinePlayManager();
  OnlinePlayManager(const OnlinePlayManager&) = delete;
  OnlinePlayManager& operator=(const OnlinePlayManager&) = delete;
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
      ENetworkTransportType TransportType,
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
