#include <EOSPlugin/EOSPlugin.h>

#include "EOSOnlinePlayManager.h"
#include "EOSTitleStorageManager.h"
namespace BEPlugins::EOS {
bool ConfigureTransport(std::string_view Name) {
  return EOSOnlinePlayManager::GetInstance().ConfigureTransport(Name);
}
const std::string& GetTransportName() {
  return EOSOnlinePlayManager::GetInstance().GetTransportName();
}
bool Login(const char* DisplayName, ResultCallback OnComplete) {
  return EOSOnlinePlayManager::GetInstance().Login(DisplayName, OnComplete);
}
bool HostLobby(const FHostLobbyRequest& Request, ResultCallback OnComplete) {
  return EOSOnlinePlayManager::GetInstance().HostLobby(Request, OnComplete);
}
bool SearchLobbies(const FLobbySearchRequest& Request, SearchCallback OnComplete) {
  return EOSOnlinePlayManager::GetInstance().SearchLobbies(Request, OnComplete);
}
bool JoinLobby(const FLobbyInfo& LobbyInfo, uint16_t Port, ResultCallback OnComplete) {
  return EOSOnlinePlayManager::GetInstance().JoinLobby(LobbyInfo, Port, OnComplete);
}
bool LeaveLobby(ResultCallback OnComplete) {
  return EOSOnlinePlayManager::GetInstance().LeaveLobby(OnComplete);
}
bool IsEOSInitialized() { return EOSOnlinePlayManager::GetInstance().IsEOSInitialized(); }
bool IsLoggedIn() { return EOSOnlinePlayManager::GetInstance().IsLoggedIn(); }
bool IsInLobby() { return EOSOnlinePlayManager::GetInstance().IsInLobby(); }
bool IsOperationPending() { return EOSOnlinePlayManager::GetInstance().IsOperationPending(); }
EOnlinePlayState GetState() { return EOSOnlinePlayManager::GetInstance().GetState(); }
bool CanLogin() { return EOSOnlinePlayManager::GetInstance().CanLogin(); }
bool CanHostLobby() { return EOSOnlinePlayManager::GetInstance().CanHostLobby(); }
bool CanSearchLobbies() { return EOSOnlinePlayManager::GetInstance().CanSearchLobbies(); }
bool CanJoinLobby() { return EOSOnlinePlayManager::GetInstance().CanJoinLobby(); }
bool CanLeaveLobby() { return EOSOnlinePlayManager::GetInstance().CanLeaveLobby(); }
std::string GetLocalUserIdString() {
  return EOSOnlinePlayManager::GetInstance().GetLocalUserIdString();
}
std::string GetCurrentLobbyId() { return EOSOnlinePlayManager::GetInstance().GetCurrentLobbyId(); }
bool Download(const std::string& FileName, CompletionCallback OnComplete) {
  return EOSTitleStorageManager::GetInstance().Download(FileName, std::move(OnComplete));
}
bool IsDownloadPending() { return EOSTitleStorageManager::GetInstance().IsDownloadPending(); }
std::string GetDownloadDirectoryPath() {
  return EOSTitleStorageManager::GetDownloadDirectoryPath();
}
}  // namespace BEPlugins::EOS
