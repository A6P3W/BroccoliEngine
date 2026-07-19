#include "OnlinePlayManager.h"

#include <algorithm>
#include <utility>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "Log.h"
#include "NetworkManager.h"
#include "NetworkUtils.h"
#include "SceneManager.h"
#include "World.h"

namespace {
const char* GetTransportTypeName(ENetworkTransportType Type) {
  return Type == ENetworkTransportType::EOSP2P ? "EOSP2P" : "ENet";
}
}  // namespace

OnlinePlayManager& OnlinePlayManager::GetInstance() {
  static OnlinePlayManager Instance;
  return Instance;
}

OnlinePlayManager::OnlinePlayManager() {
  NetworkManager::GetInstance().SetPeerAuthorizationCallback([](const std::string& ProductUserId) {
    return EOSLobbyManager::GetInstance().IsCurrentLobbyMember(ProductUserId);
  });
  EOSLobbyManager::GetInstance().SetOnLobbyDisconnected([this](ELobbyDisconnectReason Reason) {
    HandleLobbyDisconnected(Reason);
  });
  EOSAuthManager::GetInstance().SetOnAuthLost([this](EAuthLossReason Reason) {
    HandleAuthLost(Reason);
  });
  NetworkDisconnectedCallbackHandle = NetworkManager::GetInstance().AddOnDisconnected(
      [this](FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason) {
        HandleNetworkDisconnected(ConnectionId, Reason);
      }
  );
  M_LOG("[OnlinePlay] Manager initialized.");
}

OnlinePlayManager::~OnlinePlayManager() = default;

bool OnlinePlayManager::ConfigureTransport(ENetworkTransportType Type) {
  if (OperationPending || IsInLobby() || NetworkManager::GetInstance().IsRunning()) {
    M_LOG(
        "[OnlinePlay] ConfigureTransport rejected: requested={} state={} inLobby={} running={}",
        GetTransportTypeName(Type),
        static_cast<int>(State),
        IsInLobby(),
        NetworkManager::GetInstance().IsRunning()
    );
    return false;
  }
  const bool Configured = NetworkManager::GetInstance().SetTransportType(Type);
  M_LOG(
      "[OnlinePlay] ConfigureTransport completed: transport={} success={}",
      GetTransportTypeName(Type),
      Configured
  );
  return Configured;
}

ENetworkTransportType OnlinePlayManager::GetTransportType() const {
  return NetworkManager::GetInstance().GetTransportType();
}

void OnlinePlayManager::Shutdown() {
  M_LOG("[OnlinePlay] Shutdown requested.");
  BeginSessionEnd(ESessionDisconnectReason::LocalLeave, false, false);
  if (NetworkDisconnectedCallbackHandle != 0) {
    NetworkManager::GetInstance().RemoveOnDisconnected(NetworkDisconnectedCallbackHandle);
    NetworkDisconnectedCallbackHandle = 0;
  }
  EOSLobbyManager::GetInstance().SetOnLobbyDisconnected(nullptr);
  EOSAuthManager::GetInstance().SetOnAuthLost(nullptr);
}

bool OnlinePlayManager::Login(const char* DisplayName, ResultCallback OnComplete) {
  EOnlinePlayError Error = EOnlinePlayError::None;
  std::string Message;
  if (!IsEOSInitialized()) {
    Error = EOnlinePlayError::EOSNotInitialized;
    Message = "EOS is not initialized.";
  } else if (IsLoggedIn()) {
    Error = EOnlinePlayError::AlreadyLoggedIn;
    Message = "Already logged in.";
  } else if (!DisplayName || DisplayName[0] == '\0') {
    Error = EOnlinePlayError::InvalidRequest;
    Message = "Display name is empty.";
  } else if (!BeginOperation(EOnlinePlayState::LoggingIn)) {
    Error = EOnlinePlayError::OperationInProgress;
    Message = "Another operation is in progress.";
  }
  if (Error != EOnlinePlayError::None) {
    auto Result = MakeResult(false, Error, std::move(Message));
    CompleteResult(OnComplete, Result);
    return false;
  }
  M_LOG("[OnlinePlay] Login requested: displayName={}", DisplayName);
  EOSAuthManager::GetInstance().LoginWithDeviceId(
      DisplayName, [this, OnComplete = std::move(OnComplete)](bool Success) mutable {
        EndOperation();
        auto Result = MakeResult(
            Success,
            Success ? EOnlinePlayError::None : EOnlinePlayError::LoginFailed,
            Success ? "Login succeeded." : "Login failed."
        );
        M_LOG("[OnlinePlay] Login completed: success={}", Success);
        CompleteResult(OnComplete, Result);
      }
  );
  return true;
}

bool OnlinePlayManager::HostLobby(const FHostLobbyRequest& Request, ResultCallback OnComplete) {
  EOnlinePlayError Error = EOnlinePlayError::None;
  std::string Message;
  if (!IsLoggedIn()) {
    Error = EOnlinePlayError::NotLoggedIn;
    Message = "Login is required.";
  } else if (IsInLobby()) {
    Error = EOnlinePlayError::AlreadyInLobby;
    Message = "Already in a lobby.";
  } else if (Request.MaxMembers < 1 || Request.Port == 0 || Request.BucketId.empty()) {
    Error = EOnlinePlayError::InvalidRequest;
    Message = "Host request is invalid.";
  } else if (!BeginOperation(EOnlinePlayState::HostingLobby)) {
    Error = EOnlinePlayError::OperationInProgress;
    Message = "Another operation is in progress.";
  }
  if (Error != EOnlinePlayError::None) {
    auto Result = MakeResult(false, Error, std::move(Message));
    CompleteResult(OnComplete, Result);
    return false;
  }

  const ENetworkTransportType TransportType = GetTransportType();
  FCreateLobbyRequest RoutingRequest;
  RoutingRequest.MaxMembers = Request.MaxMembers;
  RoutingRequest.bPublicAdvertised = Request.PublicAdvertised;
  RoutingRequest.BucketId = Request.BucketId;
  RoutingRequest.Port = Request.Port;
  RoutingRequest.Attributes = Request.Attributes;
  if (TransportType == ENetworkTransportType::ENet) {
    RoutingRequest.HostIPAddress = NetworkUtils::GetLocalIPAddress();
    if (RoutingRequest.HostIPAddress.empty()) {
      M_LOG("[OnlinePlay] HostLobby failed: local ENet address was not found.");
      EndOperation();
      auto Result = MakeResult(
          false, EOnlinePlayError::LocalAddressNotFound, "Local IP address was not found."
      );
      CompleteResult(OnComplete, Result);
      return false;
    }
  }

  M_LOG(
      "[OnlinePlay] HostLobby requested: transport={} port={} maxMembers={} hostIP={}",
      GetTransportTypeName(TransportType),
      RoutingRequest.Port,
      RoutingRequest.MaxMembers,
      RoutingRequest.HostIPAddress.empty() ? "<unused>" : RoutingRequest.HostIPAddress
  );
  EOSLobbyManager::GetInstance().CreateLobby(
      RoutingRequest,
      [this, RoutingRequest, TransportType, OnComplete = std::move(OnComplete)](
          bool Success, const FLobbyInfo& LobbyInfo
      ) mutable {
        HandleHostLobbyCreated(
            RoutingRequest, TransportType, std::move(OnComplete), Success, LobbyInfo
        );
      }
  );
  return true;
}

void OnlinePlayManager::HandleHostLobbyCreated(
    const FCreateLobbyRequest& RoutingRequest,
    ENetworkTransportType TransportType,
    ResultCallback OnComplete,
    bool Success,
    const FLobbyInfo& LobbyInfo
) {
  if (!Success) {
    M_LOG("[OnlinePlay] HostLobby failed: EOS lobby creation failed.");
    EndOperation();
    auto Result =
        MakeResult(false, EOnlinePlayError::LobbyCreationFailed, "Lobby creation failed.");
    CompleteResult(OnComplete, Result);
    return;
  }
  if (!NetworkManager::GetInstance().StartServer(
          RoutingRequest.Port, static_cast<size_t>((std::max)(1, RoutingRequest.MaxMembers))
      )) {
    M_LOG(
        "[OnlinePlay] HostLobby failed: transport start failed. transport={} lobby={} port={}",
        GetTransportTypeName(TransportType),
        LobbyInfo.LobbyId,
        RoutingRequest.Port
    );
    RollbackLobby(
        std::move(OnComplete),
        EOnlinePlayError::TransportStartFailed,
        "Lobby was created, but host transport failed to start."
    );
    return;
  }
  if (!ApplyNetMode(ENetMode::ListenServer)) {
    M_LOG("[OnlinePlay] HostLobby failed: ListenServer NetMode could not be applied.");
    RollbackLobby(
        std::move(OnComplete),
        EOnlinePlayError::NetModeApplyFailed,
        "Host transport started, but ListenServer state could not be applied."
    );
    return;
  }
  SessionDisconnectNotified = false;
  EndOperation(EOnlinePlayState::Hosting);
  auto Result = MakeResult(true, EOnlinePlayError::None, "Lobby host is ready.", LobbyInfo);
  M_LOG(
      "[OnlinePlay] HostLobby completed: lobby={} transport={} hostIP={} port={}",
      LobbyInfo.LobbyId,
      GetTransportTypeName(TransportType),
      LobbyInfo.HostIPAddress.empty() ? "<unused>" : LobbyInfo.HostIPAddress,
      RoutingRequest.Port
  );
  CompleteResult(OnComplete, Result);
}

bool OnlinePlayManager::SearchLobbies(
    const FLobbySearchRequest& Request, SearchCallback OnComplete
) {
  EOnlinePlayError Error = EOnlinePlayError::None;
  std::string Message;
  if (!IsLoggedIn()) {
    Error = EOnlinePlayError::NotLoggedIn;
    Message = "Login is required.";
  } else if (IsInLobby()) {
    Error = EOnlinePlayError::AlreadyInLobby;
    Message = "Leave the current lobby before searching.";
  } else if (Request.MaxResults < 1 || Request.BucketId.empty()) {
    Error = EOnlinePlayError::InvalidRequest;
    Message = "Lobby search request is invalid.";
  } else if (!BeginOperation(EOnlinePlayState::SearchingLobbies)) {
    Error = EOnlinePlayError::OperationInProgress;
    Message = "Another operation is in progress.";
  }
  if (Error != EOnlinePlayError::None) {
    auto Result = MakeResult(false, Error, std::move(Message));
    if (OnComplete) OnComplete(Result, {});
    return false;
  }

  M_LOG(
      "[OnlinePlay] SearchLobbies requested: bucket={} maxResults={}",
      Request.BucketId,
      Request.MaxResults
  );
  EOSLobbyManager::GetInstance().SearchLobbies(
      Request,
      [this, OnComplete = std::move(OnComplete)](
          bool Success, const std::vector<FLobbyInfo>& Results
      ) mutable {
        EndOperation();
        auto Result = MakeResult(
            Success,
            Success ? EOnlinePlayError::None : EOnlinePlayError::LobbySearchFailed,
            Success ? "Lobby search completed." : "Lobby search failed."
        );
        M_LOG(
            "[OnlinePlay] SearchLobbies completed: success={} resultCount={}",
            Success,
            Results.size()
        );
        if (OnComplete) OnComplete(Result, Results);
      }
  );
  return true;
}

bool OnlinePlayManager::JoinLobby(
    const FLobbyInfo& LobbyInfo, uint16_t Port, ResultCallback OnComplete
) {
  EOnlinePlayError Error = EOnlinePlayError::None;
  std::string Message;
  if (!IsLoggedIn()) {
    Error = EOnlinePlayError::NotLoggedIn;
    Message = "Login is required.";
  } else if (IsInLobby()) {
    Error = EOnlinePlayError::AlreadyInLobby;
    Message = "Already in a lobby.";
  } else if (!LobbyInfo.bValid || LobbyInfo.LobbyId.empty() || Port == 0) {
    Error = EOnlinePlayError::InvalidRequest;
    Message = "Lobby selection is invalid.";
  } else if (!BeginOperation(EOnlinePlayState::JoiningLobby)) {
    Error = EOnlinePlayError::OperationInProgress;
    Message = "Another operation is in progress.";
  }
  if (Error != EOnlinePlayError::None) {
    auto Result = MakeResult(false, Error, std::move(Message));
    CompleteResult(OnComplete, Result);
    return false;
  }

  const ENetworkTransportType TransportType = GetTransportType();
  const std::string ConnectionTarget = TransportType == ENetworkTransportType::EOSP2P
                                           ? LobbyInfo.OwnerProductUserId
                                           : LobbyInfo.HostIPAddress;
  if (ConnectionTarget.empty()) {
    EndOperation();
    auto Result =
        MakeResult(false, EOnlinePlayError::InvalidRequest, "Lobby connection data is missing.");
    CompleteResult(OnComplete, Result);
    return false;
  }
  const uint16_t ConnectionPort = TransportType == ENetworkTransportType::EOSP2P ? 0 : Port;
  const std::string LocalAddress = NetworkUtils::GetLocalIPAddress();
  M_LOG(
      "[OnlinePlay] JoinLobby requested: lobby={} transport={} localIP={} target={} port={}",
      LobbyInfo.LobbyId,
      GetTransportTypeName(TransportType),
      LocalAddress.empty() ? "<unknown>" : LocalAddress,
      ConnectionTarget,
      ConnectionPort
  );
  EOSLobbyManager::GetInstance().JoinLobby(
      LobbyInfo,
      [this,
       LobbyInfo,
       ConnectionPort,
       TransportType,
       ConnectionTarget,
       OnComplete = std::move(OnComplete)](bool Success) mutable {
        if (!Success) {
          EndOperation();
          auto Result = MakeResult(false, EOnlinePlayError::LobbyJoinFailed, "Lobby join failed.");
          CompleteResult(OnComplete, Result);
          return;
        }
        if (!NetworkManager::GetInstance().ConnectToServer(ConnectionTarget, ConnectionPort)) {
          M_LOG(
              "[OnlinePlay] JoinLobby failed: transport connection failed. target={} port={}",
              ConnectionTarget,
              ConnectionPort
          );
          RollbackLobby(
              std::move(OnComplete),
              EOnlinePlayError::TransportConnectFailed,
              "Joined the lobby, but transport connection failed."
          );
          return;
        }
        if (!ApplyNetMode(ENetMode::Client)) {
          M_LOG("[OnlinePlay] JoinLobby failed: Client NetMode could not be applied.");
          RollbackLobby(
              std::move(OnComplete),
              EOnlinePlayError::NetModeApplyFailed,
              "Transport connection started, but Client state could not be applied."
          );
          return;
        }
        SessionDisconnectNotified = false;
        EndOperation(EOnlinePlayState::Connected);
        auto Result =
            MakeResult(true, EOnlinePlayError::None, "Joined lobby and connecting.", LobbyInfo);
        M_LOG(
            "[OnlinePlay] JoinLobby transport requested: lobby={} transport={} target={} port={} "
            "awaitingConnectionEvent=true",
            LobbyInfo.LobbyId,
            GetTransportTypeName(TransportType),
            ConnectionTarget,
            ConnectionPort
        );
        CompleteResult(OnComplete, Result);
      }
  );
  return true;
}

bool OnlinePlayManager::LeaveLobby(ResultCallback OnComplete) {
  if (!CanLeaveLobby()) {
    auto Result = MakeResult(
        false,
        OperationPending ? EOnlinePlayError::OperationInProgress : EOnlinePlayError::InvalidRequest,
        IsInLobby() ? "Another operation is in progress." : "Not in a lobby."
    );
    CompleteResult(OnComplete, Result);
    return false;
  }
  BeginSessionEnd(ESessionDisconnectReason::LocalLeave, true, false, std::move(OnComplete));
  return true;
}

void OnlinePlayManager::RollbackLobby(
    ResultCallback OnComplete, EOnlinePlayError Error, std::string Message
) {
  NetworkManager::GetInstance().Disconnect();
  NetworkManager::GetInstance().Stop();
  ApplyNetMode(ENetMode::Standalone);
  auto Finish = [this, Error, Message = std::move(Message), OnComplete = std::move(OnComplete)](
                    bool RollbackSucceeded
                ) mutable {
    EndOperation();
    M_LOG(
        "[OnlinePlay] Rollback completed: success={} originalError={}",
        RollbackSucceeded,
        static_cast<int>(Error)
    );
    auto Result = MakeResult(
        false,
        RollbackSucceeded ? Error : EOnlinePlayError::RollbackFailed,
        std::move(Message),
        {},
        RollbackSucceeded
    );
    CompleteResult(OnComplete, Result);
  };
  if (EOSLobbyManager::GetInstance().IsInLobby()) {
    EOSLobbyManager::GetInstance().LeaveLobby(std::move(Finish));
  } else {
    Finish(true);
  }
}

bool OnlinePlayManager::IsEOSInitialized() const {
  return EOSCoreManager::GetInstance().IsInitialized();
}
bool OnlinePlayManager::IsLoggedIn() const { return EOSAuthManager::GetInstance().IsLoggedIn(); }
bool OnlinePlayManager::IsInLobby() const { return EOSLobbyManager::GetInstance().IsInLobby(); }
bool OnlinePlayManager::IsOperationPending() const { return OperationPending; }
EOnlinePlayState OnlinePlayManager::GetState() const { return State; }
bool OnlinePlayManager::CanLogin() const {
  return IsEOSInitialized() && !IsLoggedIn() && !OperationPending;
}
bool OnlinePlayManager::CanHostLobby() const {
  return IsEOSInitialized() && IsLoggedIn() && !IsInLobby() && !OperationPending;
}
bool OnlinePlayManager::CanSearchLobbies() const { return CanHostLobby(); }
bool OnlinePlayManager::CanJoinLobby() const { return CanHostLobby(); }
bool OnlinePlayManager::CanLeaveLobby() const {
  return IsEOSInitialized() && IsLoggedIn() && IsInLobby() && !OperationPending &&
         !IsLeavingSession;
}
std::string OnlinePlayManager::GetLocalUserIdString() const {
  return EOSAuthManager::GetInstance().GetLocalUserIdString();
}
std::string OnlinePlayManager::GetCurrentLobbyId() const {
  return EOSLobbyManager::GetInstance().GetCurrentLobbyId();
}

bool OnlinePlayManager::BeginOperation(EOnlinePlayState NewState) {
  if (OperationPending) return false;
  OperationPending = true;
  State = NewState;
  M_LOG("[OnlinePlay] State changed: state={}", static_cast<int>(State));
  return true;
}

void OnlinePlayManager::EndOperation(EOnlinePlayState NewState) {
  OperationPending = false;
  State = NewState;
  M_LOG("[OnlinePlay] State changed: state={}", static_cast<int>(State));
}

FOnlinePlayResult OnlinePlayManager::MakeResult(
    bool Success,
    EOnlinePlayError Error,
    std::string Message,
    const FLobbyInfo& LobbyInfo,
    bool RollbackSucceeded
) const {
  FOnlinePlayResult Result;
  Result.Success = Success;
  Result.Error = Error;
  Result.Message = std::move(Message);
  Result.LobbyInfo = LobbyInfo;
  Result.TransportType = GetTransportType();
  Result.RollbackSucceeded = RollbackSucceeded;
  return Result;
}

void OnlinePlayManager::CompleteResult(
    ResultCallback& OnComplete, const FOnlinePlayResult& Result
) const {
  M_LOG(
      "[OnlinePlay] Operation result: success={} error={} transport={} rollback={}",
      Result.Success,
      static_cast<int>(Result.Error),
      GetTransportTypeName(Result.TransportType),
      Result.RollbackSucceeded
  );
  if (OnComplete) OnComplete(Result);
}

bool OnlinePlayManager::ApplyNetMode(ENetMode NetMode) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld) {
    M_LOG("[OnlinePlay] ApplyNetMode failed: current World is null.");
    return false;
  }
  CurrentWorld->SetNetMode(NetMode);
  M_LOG("[OnlinePlay] NetMode applied: mode={}", static_cast<int>(NetMode));
  return true;
}

void OnlinePlayManager::BeginSessionEnd(
    ESessionDisconnectReason Reason, bool LeaveEOSLobby, bool Notify, ResultCallback OnComplete
) {
  if (IsEndingSession) {
    M_LOG("[OnlinePlay] Leave already in progress: reason={}", static_cast<int>(Reason));
    auto Result = MakeResult(true, EOnlinePlayError::None, "Leave is already in progress.");
    CompleteResult(OnComplete, Result);
    return;
  }
  IsEndingSession = true;
  IsLeavingSession = true;
  OperationPending = true;
  State = EOnlinePlayState::Leaving;
  NetworkManager::GetInstance().Disconnect();
  NetworkManager::GetInstance().Stop();
  ApplyNetMode(ENetMode::Standalone);
  M_LOG(
      "[OnlinePlay] Leave started: reason={} leaveLobby={} notify={}",
      static_cast<int>(Reason),
      LeaveEOSLobby,
      Notify
  );
  auto Finish = [this, Reason, Notify, OnComplete = std::move(OnComplete)](bool Success) mutable {
    if (EOSLobbyManager::GetInstance().IsInLobby()) {
      EOSLobbyManager::GetInstance().ForceLocalDisconnect(Reason);
    }
    FinalizeSessionEnd(Reason, Success, Notify, std::move(OnComplete));
  };
  if (LeaveEOSLobby && EOSLobbyManager::GetInstance().IsInLobby() && IsLoggedIn()) {
    EOSLobbyManager::GetInstance().LeaveLobby(std::move(Finish));
  } else {
    Finish(true);
  }
}

void OnlinePlayManager::FinalizeSessionEnd(
    ESessionDisconnectReason Reason, bool Success, bool Notify, ResultCallback OnComplete
) {
  NetworkManager::GetInstance().Stop();
  ApplyNetMode(ENetMode::Standalone);
  IsLeavingSession = false;
  IsEndingSession = false;
  EndOperation();
  M_LOG(
      "[OnlinePlay] Leave completed: reason={} success={} notify={}",
      static_cast<int>(Reason),
      Success,
      Notify
  );
  if (Notify && !SessionDisconnectNotified) {
    SessionDisconnectNotified = true;
    NotifySessionDisconnected(Reason);
  }
  auto Result = MakeResult(
      Success,
      Success ? EOnlinePlayError::None : EOnlinePlayError::LobbyLeaveFailed,
      Success ? "Left online play." : "Local state was reset, but EOS lobby leave failed."
  );
  CompleteResult(OnComplete, Result);
}

void OnlinePlayManager::NotifySessionDisconnected(ELobbyDisconnectReason Reason) {
  GameInstance* CurrentGameInstance = SceneManager::GetInstance().GetGameInstance();
  if (!CurrentGameInstance) {
    M_LOG("[OnlinePlay] Disconnect notification skipped: GameInstance is null.");
    return;
  }
  CurrentGameInstance->OnSessionDisconnected(Reason);
}

void OnlinePlayManager::HandleLobbyDisconnected(ELobbyDisconnectReason Reason) {
  if (IsEndingSession) {
    M_LOG(
        "[OnlinePlay] Lobby disconnect absorbed by active leave: reason={}",
        static_cast<int>(Reason)
    );
    return;
  }
  BeginSessionEnd(Reason, false, true);
}

void OnlinePlayManager::HandleNetworkDisconnected(
    FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason
) {
  NetworkManager& Network = NetworkManager::GetInstance();
  if (!Network.IsClient() || !IsInLobby() || IsEndingSession) return;
  const ESessionDisconnectReason SessionReason = Reason == ESessionDisconnectReason::RemoteLeave
                                                     ? ESessionDisconnectReason::HostClosed
                                                     : Reason;
  M_LOG(
      "[OnlinePlay] Client transport disconnected: connection={} reason={}",
      ConnectionId,
      static_cast<int>(SessionReason)
  );
  BeginSessionEnd(SessionReason, true, true);
}

void OnlinePlayManager::HandleAuthLost(EAuthLossReason Reason) {
  M_LOG("[OnlinePlay] Auth lost: reason={}", static_cast<int>(Reason));
  BeginSessionEnd(ESessionDisconnectReason::AuthLost, false, true);
}
