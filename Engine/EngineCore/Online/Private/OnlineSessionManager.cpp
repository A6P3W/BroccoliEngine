#include "OnlineSessionManager.h"

#include <utility>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "EOSLobbyManager.h"
#include "Log.h"
#include "NetworkManager.h"

namespace {
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

OnlineSessionManager& OnlineSessionManager::Get() {
  static OnlineSessionManager Instance;
  return Instance;
}

OnlineSessionManager::OnlineSessionManager() {
  EOSLobbyManager::Get().SetOnLobbyDisconnected(
      [this](ELobbyDisconnectReason Reason) { HandleLobbyDisconnected(Reason); }
  );
  EOSAuthManager::Get().SetOnAuthLost([this](EAuthLossReason Reason) { HandleAuthLost(Reason); });
}

bool OnlineSessionManager::LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete) {
  if (!CanShowLogin() || !BeginOperation()) {
    M_LOG("[OnlineSession] LoginWithDeviceId rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  EOSAuthManager::Get().LoginWithDeviceId(DisplayName, [this, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
    EndOperation();
    CompleteBoolCallback(OnComplete, bSuccess);
  });
  return true;
}

bool OnlineSessionManager::CreateLobby(
    const FCreateLobbyRequest& Request,
    std::function<void(bool, const FLobbyInfo&)> OnComplete
) {
  if (!CanShowLobbyActions() || !BeginOperation()) {
    M_LOG("[OnlineSession] CreateLobby rejected.");
    CompleteCreateCallback(OnComplete, false, {});
    return false;
  }

  EOSLobbyManager::Get().CreateLobby(
      Request,
      [this, OnComplete = std::move(OnComplete)](bool bSuccess, const FLobbyInfo& LobbyInfo) mutable {
        EndOperation();
        CompleteCreateCallback(OnComplete, bSuccess, LobbyInfo);
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

  EOSLobbyManager::Get().SearchLobbies(
      Request,
      [this, OnComplete = std::move(OnComplete)](
          bool bSuccess,
          const std::vector<FLobbyInfo>& Results
      ) mutable {
        EndOperation();
        CompleteSearchCallback(OnComplete, bSuccess, Results);
      }
  );
  return true;
}

bool OnlineSessionManager::JoinLobby(
    const FLobbyInfo& LobbyInfo,
    uint16_t Port,
    std::function<void(bool)> OnComplete
) {
  if (!CanShowJoinLobby() || !BeginOperation()) {
    M_LOG("[OnlineSession] JoinLobby rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  if (LobbyInfo.HostIPAddress.empty()) {
    M_LOG("[OnlineSession] JoinLobby rejected: HostIPAddress is empty.");
    EndOperation();
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  EOSLobbyManager::Get().JoinLobby(
      LobbyInfo,
      [this, LobbyInfo, Port, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
        if (!bSuccess) {
          EndOperation();
          CompleteBoolCallback(OnComplete, false);
          return;
        }

        if (!NetworkManager::GetInstance().ConnectToServer(LobbyInfo.HostIPAddress, Port)) {
          M_LOG(
              "[OnlineSession] ENet ConnectToServer failed: host={}, port={}",
              LobbyInfo.HostIPAddress,
              Port
          );
          EOSLobbyManager::Get().LeaveLobby(
              [this, OnComplete = std::move(OnComplete)](bool bLeaveSuccess) mutable {
                if (!bLeaveSuccess) {
                  M_LOG("[OnlineSession] Rollback LeaveLobby failed after ENet connection failure.");
                }
                EndOperation();
                CompleteBoolCallback(OnComplete, false);
              }
          );
          return;
        }

        M_LOG(
            "[OnlineSession] Joined lobby and started ENet connection: lobby={}, host={}, port={}",
            LobbyInfo.LobbyId,
            LobbyInfo.HostIPAddress,
            Port
        );
        EndOperation();
        CompleteBoolCallback(OnComplete, true);
      }
  );
  return true;
}

bool OnlineSessionManager::LeaveLobby(std::function<void(bool)> OnComplete) {
  if (!CanShowLeaveLobby() || !BeginOperation()) {
    M_LOG("[OnlineSession] LeaveLobby rejected.");
    CompleteBoolCallback(OnComplete, false);
    return false;
  }

  EOSLobbyManager::Get().LeaveLobby([this, OnComplete = std::move(OnComplete)](bool bSuccess) mutable {
    if (bSuccess) {
      NetworkManager::GetInstance().Stop();
    }
    EndOperation();
    CompleteBoolCallback(OnComplete, bSuccess);
  });
  return true;
}

bool OnlineSessionManager::IsEOSInitialized() const { return EOSCoreManager::Get().IsInitialized(); }

bool OnlineSessionManager::IsLoggedIn() const { return EOSAuthManager::Get().IsLoggedIn(); }

bool OnlineSessionManager::IsInLobby() const { return EOSLobbyManager::Get().IsInLobby(); }

bool OnlineSessionManager::IsOperationPending() const { return bOperationPending; }

bool OnlineSessionManager::CanShowLogin() const {
  return IsEOSInitialized() && !IsLoggedIn() && !bOperationPending;
}

bool OnlineSessionManager::CanShowLobbyActions() const {
  return IsEOSInitialized() && IsLoggedIn() && !IsInLobby() && !bOperationPending;
}

bool OnlineSessionManager::CanShowSearchLobbies() const { return CanShowLobbyActions(); }

bool OnlineSessionManager::CanShowJoinLobby() const { return CanShowLobbyActions(); }

bool OnlineSessionManager::CanShowLeaveLobby() const {
  return IsEOSInitialized() && IsLoggedIn() && IsInLobby() && !bOperationPending;
}

std::string OnlineSessionManager::GetLocalUserIdString() const {
  return EOSAuthManager::Get().GetLocalUserIdString();
}

std::string OnlineSessionManager::GetCurrentLobbyId() const {
  return EOSLobbyManager::Get().GetCurrentLobbyId();
}

bool OnlineSessionManager::BeginOperation() {
  if (bOperationPending) {
    return false;
  }

  bOperationPending = true;
  return true;
}

void OnlineSessionManager::EndOperation() { bOperationPending = false; }

void OnlineSessionManager::HandleLobbyDisconnected(ELobbyDisconnectReason Reason) {
  (void)Reason;
  if (bOperationPending) {
    M_LOG("[OnlineSession] Operation was reset by lobby disconnect.");
  }
  bOperationPending = false;
}

void OnlineSessionManager::HandleAuthLost(EAuthLossReason Reason) {
  (void)Reason;
  if (EOSLobbyManager::Get().IsInLobby()) {
    EOSLobbyManager::Get().ForceLocalDisconnect(ELobbyDisconnectReason::AuthLost);
  }

  if (bOperationPending) {
    M_LOG("[OnlineSession] Operation was reset by auth loss.");
  }
  bOperationPending = false;
}
