#pragma once

#include "EOSTypes.h"

#include <functional>
#include <string>
#include <vector>

#include <eos_lobby_types.h>

class EOSLobbyManager {
 public:
  static EOSLobbyManager& Get();

  void CreateLobby(const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete);
  void LeaveLobby(std::function<void(bool)> OnComplete);
  void SearchLobbies(
      const FLobbySearchRequest& Request,
      std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
  );
  void JoinLobby(const FLobbyInfo& LobbyInfo, std::function<void(bool)> OnComplete);
  void ForceLocalDisconnect(ELobbyDisconnectReason Reason);
  void SetOnLobbyDisconnected(std::function<void(ELobbyDisconnectReason)> Callback);
  void Shutdown();

  bool IsInLobby() const;
  std::string GetCurrentLobbyId() const;

 private:
  struct FCachedLobbyDetails {
    std::string LobbyId;
    EOS_HLobbyDetails DetailsHandle = nullptr;
  };

 private:
  EOSLobbyManager() = default;
  ~EOSLobbyManager();

  EOSLobbyManager(const EOSLobbyManager&) = delete;
  EOSLobbyManager& operator=(const EOSLobbyManager&) = delete;

 private:
  void ClearCachedLobbyDetails();
  EOS_HLobbyDetails FindCachedLobbyDetails(const std::string& LobbyId) const;
  void RegisterLobbyNotifications();
  void UnregisterLobbyNotifications();
  void HandleLocalDisconnect(ELobbyDisconnectReason Reason, bool bNotify);
  void HandleMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);
  void HandleLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);
  bool IsCurrentLobbyUpdateClosed(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data) const;
  static void EOS_CALL OnLobbyMemberStatusReceived(
      const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data
  );
  static void EOS_CALL OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);

 private:
  std::string CurrentLobbyId;
  bool bInLobby = false;
  std::vector<FCachedLobbyDetails> CachedLobbyDetails;
  EOS_NotificationId MemberStatusNotificationId = EOS_INVALID_NOTIFICATIONID;
  EOS_NotificationId LobbyUpdateNotificationId = EOS_INVALID_NOTIFICATIONID;
  std::function<void(ELobbyDisconnectReason)> OnLobbyDisconnected;
};
