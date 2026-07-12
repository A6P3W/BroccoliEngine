#pragma once
#include "BroccoliEngineAPI.h"

#include "EOSTypes.h"

#include <functional>
#include <string>

#include <eos_common.h>
#include <eos_connect_types.h>

enum class EEOSAuthState {
  NotLoggedIn,
  CreatingDeviceId,
  LoggingIn,
  LoggedIn,
  Failed
};

class BROCCOLI_ENGINE_API EOSAuthManager {
 public:
  static EOSAuthManager& Get();

  void LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  void SetOnAuthLost(std::function<void(EAuthLossReason)> Callback);
  void Shutdown();

  bool IsLoggedIn() const;
  EOS_ProductUserId GetLocalUserId() const;
  std::string GetLocalUserIdString() const;

 private:
  EOSAuthManager() = default;
  ~EOSAuthManager();

  EOSAuthManager(const EOSAuthManager&) = delete;
  EOSAuthManager& operator=(const EOSAuthManager&) = delete;

 private:
  void CreateDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  void LoginAfterDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  void RegisterLoginStatusNotification();
  void UnregisterLoginStatusNotification();
  void HandleLoginStatusChanged(const EOS_Connect_LoginStatusChangedCallbackInfo* Data);
  static void EOS_CALL OnLoginStatusChanged(const EOS_Connect_LoginStatusChangedCallbackInfo* Data);

 private:
  EOS_ProductUserId LocalUserId = nullptr;
  EEOSAuthState State = EEOSAuthState::NotLoggedIn;
  EOS_NotificationId LoginStatusNotificationId = EOS_INVALID_NOTIFICATIONID;
  std::function<void(EAuthLossReason)> OnAuthLost;
};
