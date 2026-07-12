#pragma once
#include "BroccoliEngineAPI.h"

#include "EOSTypes.h"

#include <functional>
#include <string>
#include <vector>

#include <eos_lobby_types.h>

class BROCCOLI_ENGINE_API EOSLobbyManager {
 public:
  static EOSLobbyManager& Get();

  void CreateLobby(const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete);
  void UpdateCurrentLobbyAttributes(
      const std::vector<FLobbyAttribute>& Attributes,
      std::function<void(bool)> OnComplete
  );
  void LeaveLobby(std::function<void(bool)> OnComplete);
  void SearchLobbies(
      const FLobbySearchRequest& Request,
      std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
  );
  void FetchLobbyInfoById(
      const std::string& LobbyId,
      std::function<void(bool, const FLobbyInfo&)> OnComplete
  );
  void JoinLobby(const FLobbyInfo& LobbyInfo, std::function<void(bool)> OnComplete);
  void ForceLocalDisconnect(ELobbyDisconnectReason Reason);
  void SetOnLobbyDisconnected(std::function<void(ELobbyDisconnectReason)> Callback);
  void Shutdown();

  bool IsInLobby() const;
  std::string GetCurrentLobbyId() const;

  EOSLobbyManager();
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
   struct Impl;
   Impl* ImplPtr = nullptr;
 };
