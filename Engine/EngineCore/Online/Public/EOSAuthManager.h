#pragma once

#include <functional>
#include <string>

#include <eos_common.h>

enum class EEOSAuthState {
  NotLoggedIn,
  CreatingDeviceId,
  LoggingIn,
  LoggedIn,
  Failed
};

class EOSAuthManager {
 public:
  static EOSAuthManager& Get();

  void LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);

  bool IsLoggedIn() const;
  EOS_ProductUserId GetLocalUserId() const;
  std::string GetLocalUserIdString() const;

 private:
  EOSAuthManager() = default;
  ~EOSAuthManager() = default;

  EOSAuthManager(const EOSAuthManager&) = delete;
  EOSAuthManager& operator=(const EOSAuthManager&) = delete;

 private:
  void CreateDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);
  void LoginAfterDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete);

 private:
  EOS_ProductUserId LocalUserId = nullptr;
  EEOSAuthState State = EEOSAuthState::NotLoggedIn;
};
