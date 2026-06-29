#include "EOSAuthManager.h"

#include <array>
#include <utility>

#include <eos_connect.h>

#include "EOSCoreManager.h"
#include "Log.h"

namespace {
const char* SafeText(const char* Text) { return Text ? Text : "<null>"; }

const char* SafeDisplayName(const char* DisplayName) {
  return DisplayName && DisplayName[0] != '\0' ? DisplayName : "BroccoliPlayer";
}

const char* SafeEOSResult(EOS_EResult Result) { return SafeText(EOS_EResult_ToString(Result)); }

struct FCreateDeviceIdContext {
  std::string DisplayName;
  std::function<void(bool)> OnComplete;
};

struct FConnectLoginContext {
  std::string DisplayName;
  std::function<void(bool)> OnComplete;
};

void CompleteAuthCallback(std::function<void(bool)>& OnComplete, bool bSuccess) {
  if (OnComplete) {
    OnComplete(bSuccess);
  }
}
}  // namespace

EOSAuthManager& EOSAuthManager::Get() {
  static EOSAuthManager Instance;
  return Instance;
}

void EOSAuthManager::LoginWithDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete) {
  if (!EOSCoreManager::Get().IsInitialized()) {
    M_LOG("[EOSAuth] LoginWithDeviceId failed: EOSCoreManager is not initialized.");
    LocalUserId = nullptr;
    State = EEOSAuthState::Failed;
    CompleteAuthCallback(OnComplete, false);
    return;
  }

  if (!EOSCoreManager::Get().GetConnectHandle()) {
    M_LOG("[EOSAuth] LoginWithDeviceId failed: ConnectHandle is null.");
    LocalUserId = nullptr;
    State = EEOSAuthState::Failed;
    CompleteAuthCallback(OnComplete, false);
    return;
  }

  CreateDeviceId(SafeDisplayName(DisplayName), std::move(OnComplete));
}

bool EOSAuthManager::IsLoggedIn() const { return State == EEOSAuthState::LoggedIn && LocalUserId != nullptr; }

EOS_ProductUserId EOSAuthManager::GetLocalUserId() const { return LocalUserId; }

std::string EOSAuthManager::GetLocalUserIdString() const {
  if (!LocalUserId) {
    return {};
  }

  std::array<char, EOS_PRODUCTUSERID_MAX_LENGTH + 1> Buffer = {};
  int32_t BufferLength = static_cast<int32_t>(Buffer.size());
  EOS_EResult Result = EOS_ProductUserId_ToString(LocalUserId, Buffer.data(), &BufferLength);
  if (Result != EOS_EResult::EOS_Success) {
    M_LOG("[EOSAuth] ProductUserId stringify failed: {}", SafeEOSResult(Result));
    return {};
  }

  return std::string(Buffer.data());
}

void EOSAuthManager::CreateDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete) {
  EOS_HConnect ConnectHandle = EOSCoreManager::Get().GetConnectHandle();
  if (!ConnectHandle) {
    M_LOG("[EOSAuth] CreateDeviceId failed: ConnectHandle is null.");
    LocalUserId = nullptr;
    State = EEOSAuthState::Failed;
    CompleteAuthCallback(OnComplete, false);
    return;
  }

  State = EEOSAuthState::CreatingDeviceId;

  auto* Context = new FCreateDeviceIdContext{SafeDisplayName(DisplayName), std::move(OnComplete)};

  EOS_Connect_CreateDeviceIdOptions Options = {};
  Options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
  Options.DeviceModel = Context->DisplayName.c_str();
  EOS_Connect_CreateDeviceId(
      ConnectHandle,
      &Options,
      Context,
      [](const EOS_Connect_CreateDeviceIdCallbackInfo* Data) {
        auto* Context = static_cast<FCreateDeviceIdContext*>(Data ? Data->ClientData : nullptr);
        EOSAuthManager& AuthManager = EOSAuthManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSAuth] CreateDeviceId failed: callback data is null.");
          AuthManager.LocalUserId = nullptr;
          AuthManager.State = EEOSAuthState::Failed;
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success) {
          M_LOG("[EOSAuth] CreateDeviceId success");
          AuthManager.LoginAfterDeviceId(Context->DisplayName.c_str(), std::move(Context->OnComplete));
          delete Context;
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed) {
          M_LOG("[EOSAuth] DeviceId already exists");
          AuthManager.LoginAfterDeviceId(Context->DisplayName.c_str(), std::move(Context->OnComplete));
          delete Context;
          return;
        }

        M_LOG("[EOSAuth] CreateDeviceId failed: {}", SafeEOSResult(Data->ResultCode));
        AuthManager.LocalUserId = nullptr;
        AuthManager.State = EEOSAuthState::Failed;
        CompleteAuthCallback(Context->OnComplete, false);
        delete Context;
      }
  );
}

void EOSAuthManager::LoginAfterDeviceId(const char* DisplayName, std::function<void(bool)> OnComplete) {
  EOS_HConnect ConnectHandle = EOSCoreManager::Get().GetConnectHandle();
  if (!ConnectHandle) {
    M_LOG("[EOSAuth] Connect Login failed: ConnectHandle is null.");
    LocalUserId = nullptr;
    State = EEOSAuthState::Failed;
    CompleteAuthCallback(OnComplete, false);
    return;
  }

  State = EEOSAuthState::LoggingIn;

  EOS_Connect_Credentials Credentials = {};
  Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
  Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
  Credentials.Token = nullptr;

  auto* Context = new FConnectLoginContext{SafeDisplayName(DisplayName), std::move(OnComplete)};

  EOS_Connect_UserLoginInfo UserLoginInfo = {};
  UserLoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
  UserLoginInfo.DisplayName = Context->DisplayName.c_str();

  EOS_Connect_LoginOptions Options = {};
  Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
  Options.Credentials = &Credentials;
  Options.UserLoginInfo = &UserLoginInfo;
  EOS_Connect_Login(
      ConnectHandle,
      &Options,
      Context,
      [](const EOS_Connect_LoginCallbackInfo* Data) {
        auto* Context = static_cast<FConnectLoginContext*>(Data ? Data->ClientData : nullptr);
        EOSAuthManager& AuthManager = EOSAuthManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSAuth] Connect Login failed: callback data is null.");
          AuthManager.LocalUserId = nullptr;
          AuthManager.State = EEOSAuthState::Failed;
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success && Data->LocalUserId) {
          AuthManager.LocalUserId = Data->LocalUserId;
          AuthManager.State = EEOSAuthState::LoggedIn;
          M_LOG("[EOSAuth] Connect Login success");
          M_LOG("[EOSAuth] ProductUserId = {}", AuthManager.GetLocalUserIdString());
          CompleteAuthCallback(Context->OnComplete, true);
          delete Context;
          return;
        }

        M_LOG("[EOSAuth] Connect Login failed: {}", SafeEOSResult(Data->ResultCode));
        AuthManager.LocalUserId = nullptr;
        AuthManager.State = EEOSAuthState::Failed;
        CompleteAuthCallback(Context->OnComplete, false);
        delete Context;
      }
  );
}
