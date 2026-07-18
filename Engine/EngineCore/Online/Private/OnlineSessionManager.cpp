#include "OnlineSessionManager.h"

#include <algorithm>
#include <utility>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "Log.h"
#include "NetworkManager.h"
#include "SceneManager.h"

namespace {
const char* GetTransportTypeName(ENetworkTransportType Type) {
  return Type == ENetworkTransportType::EOSP2P ? "EOSP2P" : "ENet";
}

void CompleteBoolCallback(std::function<void(bool)>& OnComplete, bool bSuccess) {
  if (OnComplete) {
    OnComplete(bSuccess);
  }
}

void CompleteCreateCallback(
    std::function<void(bool, const FLobbyInfo&)>& OnComplete,
    bool bSuccess,
    const FLobbyInfo& LobbyInfo
) {
  if (OnComplete) {
    OnComplete(bSuccess, LobbyInfo);
  }
}

void CompleteSearchCallback(
    std::function<void(bool, const std::vector<FLobbyInfo>&)>& OnComplete,
    bool bSuccess,
    const std::vector<FLobbyInfo>& Results
) {
  if (OnComplete) {
    OnComplete(bSuccess, Results);
  }
}
}  // namespace

OnlineSessionManager& OnlineSessionManager::GetInstance() {
  static OnlineSessionManager Instance;
  return Instance;
}

OnlineSessionManager::OnlineSessionManager() {
  NetworkManager::GetInstance().SetPeerAuthorizationCallback([](const std::string& ProductUserId) {
    return EOSLobbyManager::GetInstance().IsCurrentLobbyMember(ProductUserId);
  });
  EOSLobbyManager::GetInstance().SetOnLobbyDisconnected([this](ELobbyDisconnectReason Reason) {
    HandleLobbyDisconnected(Reason);
  });
  EOSAuthManager::GetInstance().SetOnAuthLost([this](EAuthLossReason Reason) { HandleAuthLost(Reason); });
  NetworkDisconnectedCallbackHandle = NetworkManager::GetInstance().AddOnDisconnected(
      [this](FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason) {
        HandleNetworkDisconnected(ConnectionId, Reason);
      }
  );
}

OnlineSessionManager::~OnlineSessionManager() = default;

bool OnlineSessionManager::SetTransportType(ENetworkTransportType Type) {
  if (bOperationPending || IsInLobby()) {
    M_LOG(
        "[OnlineSession] Transport change rejected: operationPending={} inLobby={}",
        bOperationPending,
        IsInLobby()
    );
    return false;
  }
  return NetworkManager::GetInstance().SetTransportType(Type);
}
void OnlineSessionManager::Shutdown() {
  BeginSessionEnd(ESessionDisconnectReason::LocalLeave, false, false);
  if (NetworkDisconnectedCallbackHandle != 0) {
    NetworkManager::GetInstance().RemoveOnDisconnected(NetworkDisconnectedCallbackHandle);
    NetworkDisconnectedCallbackHandle = 0;
  }
  EOSLobbyManager::GetInstance().SetOnLobbyDisconnected(nullptr);
  EOSAuthManager::GetInstance().SetOnAuthLost(nullptr);
}

bool OnlineSessionManager::LoginWithDeviceId(
    const char* DisplayName, std::function<void(bool)> OnComplete
) {
  if (!CanShowLogin() || !BeginOperation()) {
    M_LOG("[OnlineSession] LoginWithDeviceId rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  EOSAuthManager::GetInstance().LoginWithDeviceId(
      DisplayName, [this, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
        EndOperation();
        CompleteBoolCallback(OnComplete, bSuccess);
      }
  );
  return true;
}

bool OnlineSessionManager::CreateLobby(
    const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete
) {
  if (!CanShowLobbyActions() || !BeginOperation()) {
    M_LOG("[OnlineSession] CreateLobby rejected.");
    CompleteCreateCallback(OnComplete, false, {});
    return false;
  }

  NetworkManager& Network = NetworkManager::GetInstance();
  const ENetworkTransportType TransportType = Network.GetTransportType();
  FCreateLobbyRequest RoutingRequest = Request;
  if (TransportType == ENetworkTransportType::EOSP2P) {
    RoutingRequest.HostIPAddress.clear();
  } else if (RoutingRequest.HostIPAddress.empty()) {
    M_LOG("[OnlineSession] CreateLobby rejected: ENet HostIPAddress is empty.");
    EndOperation();
    CompleteCreateCallback(OnComplete, false, {});
    return false;
  }

  M_LOG(
      "[OnlineSession] CreateLobby routing selected: transport={} port={} maxMembers={}",
      GetTransportTypeName(TransportType),
      RoutingRequest.Port,
      RoutingRequest.MaxMembers
  );
  EOSLobbyManager::GetInstance().CreateLobby(
      RoutingRequest,
      [this, RoutingRequest, TransportType, OnComplete = std::move(OnComplete)](
          bool bSuccess, const FLobbyInfo& LobbyInfo
      ) mutable {
        HandleCreateLobbyComplete(
            RoutingRequest, TransportType, std::move(OnComplete), bSuccess, LobbyInfo
        );
      }
  );
  return true;
}

void OnlineSessionManager::HandleCreateLobbyComplete(
    const FCreateLobbyRequest& RoutingRequest,
    ENetworkTransportType TransportType,
    std::function<void(bool, const FLobbyInfo&)> OnComplete,
    bool bSuccess,
    const FLobbyInfo& LobbyInfo
) {
  if (!bSuccess) {
    if (EOSLobbyManager::GetInstance().IsInLobby()) {
      EOSLobbyManager::GetInstance().LeaveLobby(
          [this, OnComplete = std::move(OnComplete)](bool bLeaveSuccess) mutable {
            if (!bLeaveSuccess) {
              M_LOG("[OnlineSession] Rollback LeaveLobby failed after lobby setup failure.");
            }
            NetworkManager::GetInstance().Stop();
            EndOperation();
            CompleteCreateCallback(OnComplete, false, {});
          }
      );
    } else {
      NetworkManager::GetInstance().Stop();
      EndOperation();
      CompleteCreateCallback(OnComplete, false, {});
    }
    return;
  }

  NetworkManager::GetInstance().SetTransportType(TransportType);
  if (!NetworkManager::GetInstance().StartServer(
          RoutingRequest.Port, static_cast<size_t>((std::max)(1, RoutingRequest.MaxMembers))
      )) {
    M_LOG(
        "[OnlineSession] Host transport start failed: transport={} lobby={} port={}",
        GetTransportTypeName(TransportType),
        LobbyInfo.LobbyId,
        RoutingRequest.Port
    );
    EOSLobbyManager::GetInstance().LeaveLobby(
        [this, OnComplete = std::move(OnComplete)](bool bLeaveSuccess) mutable {
          if (!bLeaveSuccess) {
            M_LOG("[OnlineSession] Rollback LeaveLobby failed after host start failure.");
          }
          NetworkManager::GetInstance().Stop();
          EndOperation();
          CompleteCreateCallback(OnComplete, false, {});
        }
    );
    return;
  }

  M_LOG(
      "[OnlineSession] Lobby host ready: transport={} lobby={} owner={} hostIP={} port={}",
      GetTransportTypeName(TransportType),
      LobbyInfo.LobbyId,
      LobbyInfo.OwnerProductUserId,
      LobbyInfo.HostIPAddress.empty() ? "<unused>" : LobbyInfo.HostIPAddress,
      RoutingRequest.Port
  );
  bSessionDisconnectNotified = false;
  M_LOG("[OnlineSession] Disconnect notification guard reset for hosted session.");
  EndOperation();
  CompleteCreateCallback(OnComplete, true, LobbyInfo);
}

bool OnlineSessionManager::UpdateCurrentLobbyAttributes(
    const std::vector<FLobbyAttribute>& Attributes, std::function<void(bool)> OnComplete
) {
  if (!CanShowLeaveSession() || !BeginOperation()) {
    M_LOG("[OnlineSession] UpdateCurrentLobbyAttributes rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  EOSLobbyManager::GetInstance().UpdateCurrentLobbyAttributes(
      Attributes, [this, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
        EndOperation();
        CompleteBoolCallback(OnComplete, bSuccess);
      }
  );
  return true;
}

bool OnlineSessionManager::SearchLobbies(
    const FLobbySearchRequest& Request,
    std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
) {
  if (!CanShowSearchLobbies() || !BeginOperation()) {
    M_LOG("[OnlineSession] SearchLobbies rejected.");
    CompleteSearchCallback(OnComplete, false, {});
    return false;
  }

  EOSLobbyManager::GetInstance().SearchLobbies(
      Request,
      [this, OnComplete = std::move(OnComplete)](
          bool bSuccess, const std::vector<FLobbyInfo>& Results
      ) mutable {
        EndOperation();
        CompleteSearchCallback(OnComplete, bSuccess, Results);
      }
  );
  return true;
}

bool OnlineSessionManager::FetchLobbyInfoById(
    const std::string& LobbyId, std::function<void(bool, const FLobbyInfo&)> OnComplete
) {
  if (!CanShowJoinLobby() || !BeginOperation()) {
    M_LOG("[OnlineSession] FetchLobbyInfoById rejected.");
    CompleteCreateCallback(OnComplete, false, {});
    return false;
  }

  EOSLobbyManager::GetInstance().FetchLobbyInfoById(
      LobbyId,
      [this,
       OnComplete = std::move(OnComplete)](bool bSuccess, const FLobbyInfo& LobbyInfo) mutable {
        EndOperation();
        CompleteCreateCallback(OnComplete, bSuccess, LobbyInfo);
      }
  );
  return true;
}

bool OnlineSessionManager::JoinLobby(
    const FLobbyInfo& LobbyInfo, uint16_t Port, std::function<void(bool)> OnComplete
) {
  if (!CanShowJoinLobby() || !BeginOperation()) {
    M_LOG("[OnlineSession] JoinLobby rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  const ENetworkTransportType TransportType = NetworkManager::GetInstance().GetTransportType();
  const std::string ConnectionTarget = TransportType == ENetworkTransportType::EOSP2P
                                           ? LobbyInfo.OwnerProductUserId
                                           : LobbyInfo.HostIPAddress;
  if (ConnectionTarget.empty()) {
    M_LOG(
        "[OnlineSession] JoinLobby rejected: connection target is empty. transport={} lobby={}",
        GetTransportTypeName(TransportType),
        LobbyInfo.LobbyId
    );
    EndOperation();
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  M_LOG(
      "[OnlineSession] JoinLobby routing selected: transport={} lobby={} target={} port={}",
      GetTransportTypeName(TransportType),
      LobbyInfo.LobbyId,
      ConnectionTarget,
      TransportType == ENetworkTransportType::EOSP2P ? 0 : Port
  );
  EOSLobbyManager::GetInstance().JoinLobby(
      LobbyInfo,
      [this, LobbyInfo, Port, TransportType, ConnectionTarget, OnComplete = std::move(OnComplete)](
          bool bSuccess
      ) mutable {
        if (!bSuccess) {
          EndOperation();
          CompleteBoolCallback(OnComplete, false);
          return;
        }

        const uint16_t ConnectionPort = TransportType == ENetworkTransportType::EOSP2P ? 0 : Port;
        NetworkManager::GetInstance().SetTransportType(TransportType);
        if (!NetworkManager::GetInstance().ConnectToServer(ConnectionTarget, ConnectionPort)) {
          M_LOG(
              "[OnlineSession] Transport connection start failed: transport={} target={} port={}",
              GetTransportTypeName(TransportType),
              ConnectionTarget,
              ConnectionPort
          );
          NetworkManager::GetInstance().Stop();
          EOSLobbyManager::GetInstance().LeaveLobby(
              [this, OnComplete = std::move(OnComplete)](bool bLeaveSuccess) mutable {
                if (!bLeaveSuccess) {
                  M_LOG("[OnlineSession] Rollback LeaveLobby failed after connection failure.");
                }
                NetworkManager::GetInstance().Stop();
                EndOperation();
                CompleteBoolCallback(OnComplete, false);
              }
          );
          return;
        }

        M_LOG(
            "[OnlineSession] Joined lobby and started transport connection: transport={} "
            "lobby={} target={} port={}",
            GetTransportTypeName(TransportType),
            LobbyInfo.LobbyId,
            ConnectionTarget,
            ConnectionPort
        );
        bSessionDisconnectNotified = false;
        M_LOG("[OnlineSession] Disconnect notification guard reset for joined session.");
        EndOperation();
        CompleteBoolCallback(OnComplete, true);
      }
  );
  return true;
}

bool OnlineSessionManager::LeaveSession(std::function<void(bool)> OnComplete) {
  if (!CanShowLeaveSession()) {
    M_LOG("[OnlineSession] LeaveSession rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }
  BeginSessionEnd(ESessionDisconnectReason::LocalLeave, true, true, std::move(OnComplete));
  return true;
}

bool OnlineSessionManager::LeaveLobby(std::function<void(bool)> OnComplete) {
  return LeaveSession(std::move(OnComplete));
}

bool OnlineSessionManager::IsEOSInitialized() const {
  return EOSCoreManager::GetInstance().IsInitialized();
}

bool OnlineSessionManager::IsLoggedIn() const { return EOSAuthManager::GetInstance().IsLoggedIn(); }

bool OnlineSessionManager::IsInLobby() const { return EOSLobbyManager::GetInstance().IsInLobby(); }

bool OnlineSessionManager::IsOperationPending() const { return bOperationPending; }

bool OnlineSessionManager::CanShowLogin() const {
  return IsEOSInitialized() && !IsLoggedIn() && !bOperationPending;
}

bool OnlineSessionManager::CanShowLobbyActions() const {
  return IsEOSInitialized() && IsLoggedIn() && !IsInLobby() && !bOperationPending;
}

bool OnlineSessionManager::CanShowSearchLobbies() const { return CanShowLobbyActions(); }

bool OnlineSessionManager::CanShowJoinLobby() const { return CanShowLobbyActions(); }

bool OnlineSessionManager::CanShowLeaveSession() const {
  return IsEOSInitialized() && IsLoggedIn() && IsInLobby() && !bOperationPending &&
         !bIsLeavingSession;
}

bool OnlineSessionManager::CanShowLeaveLobby() const { return CanShowLeaveSession(); }

std::string OnlineSessionManager::GetLocalUserIdString() const {
  return EOSAuthManager::GetInstance().GetLocalUserIdString();
}

std::string OnlineSessionManager::GetCurrentLobbyId() const {
  return EOSLobbyManager::GetInstance().GetCurrentLobbyId();
}

bool OnlineSessionManager::BeginOperation() {
  if (bOperationPending) {
    return false;
  }

  bOperationPending = true;
  return true;
}

void OnlineSessionManager::EndOperation() { bOperationPending = false; }

void OnlineSessionManager::BeginSessionEnd(
    ESessionDisconnectReason Reason,
    bool bLeaveLobby,
    bool bNotify,
    std::function<void(bool)> OnComplete
) {
  if (bIsEndingSession) {
    M_LOG("[OnlineSession] Session end already in progress: reason={}", static_cast<int>(Reason));
    CompleteBoolCallback(OnComplete, true);
    return;
  }

  bIsEndingSession = true;
  bIsLeavingSession = true;
  if (bOperationPending) {
    M_LOG("[OnlineSession] Pending operation cancelled by session end.");
  }
  bOperationPending = true;

  NetworkManager::GetInstance().Disconnect();
  NetworkManager::GetInstance().Stop();
  M_LOG(
      "[OnlineSession] Session end started: reason={} leaveLobby={}",
      static_cast<int>(Reason),
      bLeaveLobby
  );

  auto Finish = [this, Reason, bNotify, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
    if (EOSLobbyManager::GetInstance().IsInLobby()) {
      EOSLobbyManager::GetInstance().ForceLocalDisconnect(Reason);
    }
    FinalizeSessionEnd(Reason, bSuccess, bNotify, std::move(OnComplete));
  };

  if (bLeaveLobby && EOSLobbyManager::GetInstance().IsInLobby() && IsLoggedIn()) {
    EOSLobbyManager::GetInstance().LeaveLobby(std::move(Finish));
    return;
  }
  Finish(true);
}

void OnlineSessionManager::FinalizeSessionEnd(
    ESessionDisconnectReason Reason,
    bool bSuccess,
    bool bNotify,
    std::function<void(bool)> OnComplete
) {
  NetworkManager::GetInstance().Stop();
  bIsLeavingSession = false;
  bIsEndingSession = false;
  EndOperation();
  M_LOG(
      "[OnlineSession] Session end completed: reason={} success={} notify={}",
      static_cast<int>(Reason),
      bSuccess,
      bNotify
  );
  if (bNotify && !bSessionDisconnectNotified) {
    bSessionDisconnectNotified = true;
    NotifySessionDisconnected(Reason);
  }
  CompleteBoolCallback(OnComplete, bSuccess);
}

void OnlineSessionManager::NotifySessionDisconnected(ELobbyDisconnectReason Reason) {
  GameInstance* CurrentGameInstance = SceneManager::GetInstance().GetGameInstance();
  if (!CurrentGameInstance) {
    M_LOG("[OnlineSession] Session disconnected but GameInstance is null.");
    return;
  }

  CurrentGameInstance->OnSessionDisconnected(Reason);
}

void OnlineSessionManager::HandleLobbyDisconnected(ELobbyDisconnectReason Reason) {
  if (bIsEndingSession) {
    M_LOG(
        "[OnlineSession] Lobby disconnect absorbed by active session end: reason={}",
        static_cast<int>(Reason)
    );
    return;
  }
  BeginSessionEnd(Reason, false, true);
}

void OnlineSessionManager::HandleNetworkDisconnected(
    FNetworkConnectionId ConnectionId, ESessionDisconnectReason Reason
) {
  NetworkManager& Network = NetworkManager::GetInstance();
  if (!Network.IsClient() || !EOSLobbyManager::GetInstance().IsInLobby() || bIsEndingSession) {
    return;
  }
  const ESessionDisconnectReason SessionReason = Reason == ESessionDisconnectReason::RemoteLeave
                                                     ? ESessionDisconnectReason::HostClosed
                                                     : Reason;
  M_LOG(
      "[OnlineSession] Client transport disconnected: connection={} reason={}",
      ConnectionId,
      static_cast<int>(SessionReason)
  );
  BeginSessionEnd(SessionReason, true, true);
}

void OnlineSessionManager::HandleAuthLost(EAuthLossReason Reason) {
  M_LOG("[OnlineSession] Auth lost: authReason={}", static_cast<int>(Reason));
  BeginSessionEnd(ESessionDisconnectReason::AuthLost, false, true);
}
