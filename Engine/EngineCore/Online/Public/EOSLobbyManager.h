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

 private:
  std::string CurrentLobbyId;
  bool bInLobby = false;
  std::vector<FCachedLobbyDetails> CachedLobbyDetails;
};
