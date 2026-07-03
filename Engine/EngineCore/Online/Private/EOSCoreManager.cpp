#include "EOSCoreManager.h"

#include <eos_logging.h>

#include <cstring>

#include "Log.h"

#if __has_include(<EOS_ProductCredentials.h>)
#include <EOS_ProductCredentials.h>
#define HAS_EOS_CREDENTIALS 1
#else
#define HAS_EOS_CREDENTIALS 0
struct SampleConstants {
  static constexpr const char* GameName = "";
  static constexpr const char* ProductId = "";
  static constexpr const char* SandboxId = "";
  static constexpr const char* DeploymentId = "";
  static constexpr const char* ClientCredentialsId = "";
  static constexpr const char* ClientCredentialsSecret = "";
  static constexpr const char* EncryptionKey = "";
};
#endif

namespace {
const char* SafeEOSString(const char* Text) { return Text ? Text : "<null>"; }
bool IsNullOrEmpty(const char* Text) { return Text == nullptr || Text[0] == '\0'; }
void EOS_CALL HandleEOSLogMessage(const EOS_LogMessage* Message) {
  if (!Message) {
    M_LOG("[EOS] category=<null> level=<null> message=<null>");
    return;
  }
  M_LOG(
      "[EOS] category={} level={} message={}",
      SafeEOSString(Message->Category),
      static_cast<int>(Message->Level),
      SafeEOSString(Message->Message)
  );
}
}  // namespace

EOSCoreManager& EOSCoreManager::Get() {
  static EOSCoreManager Instance;
  return Instance;
}

bool EOSCoreManager::InitializeOnlineServices(const char* ProductVersion) {
#if HAS_EOS_CREDENTIALS
  FEOSConfig Config = {};
  Config.ProductName = SampleConstants::GameName;
  Config.ProductVersion = ProductVersion;
  Config.ProductId = SampleConstants::ProductId;
  Config.SandboxId = SampleConstants::SandboxId;
  Config.DeploymentId = SampleConstants::DeploymentId;
  Config.ClientId = SampleConstants::ClientCredentialsId;
  Config.ClientSecret = SampleConstants::ClientCredentialsSecret;
  Config.EncryptionKey = SampleConstants::EncryptionKey;

  return Initialize(Config);
#else
  M_LOG("EOS_ProductCredentials.h not found. EOS initialization skipped.");
  return false;
#endif
}

bool EOSCoreManager::Initialize(const FEOSConfig& Config) {
  if (bInitialized) {
    M_LOG("EOSCoreManager already initialized.");
    return true;
  }
  if (IsNullOrEmpty(Config.ProductName) || IsNullOrEmpty(Config.ProductVersion)) {
    M_LOG("EOS initialize skipped: ProductName or ProductVersion is empty.");
    return false;
  }
  if (IsNullOrEmpty(Config.ProductId) || IsNullOrEmpty(Config.SandboxId) ||
      IsNullOrEmpty(Config.DeploymentId) || IsNullOrEmpty(Config.ClientId) ||
      IsNullOrEmpty(Config.ClientSecret)) {
    M_LOG("EOS initialize skipped: platform credentials are incomplete.");
    return false;
  }
  EOS_InitializeOptions InitializeOptions = {};
  InitializeOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
  InitializeOptions.ProductName = Config.ProductName;
  InitializeOptions.ProductVersion = Config.ProductVersion;
  EOS_EResult InitializeResult = EOS_Initialize(&InitializeOptions);
  M_LOG("EOS_Initialize result: {}", SafeEOSString(EOS_EResult_ToString(InitializeResult)));
  if (InitializeResult != EOS_EResult::EOS_Success) {
    return false;
  }
  bSDKInitialized = true;
  EOS_EResult LogCallbackResult = EOS_Logging_SetCallback(HandleEOSLogMessage);
  M_LOG(
      "EOS_Logging_SetCallback result: {}", SafeEOSString(EOS_EResult_ToString(LogCallbackResult))
  );
  EOS_EResult LogLevelResult =
      EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Info);
  M_LOG("EOS_Logging_SetLogLevel result: {}", SafeEOSString(EOS_EResult_ToString(LogLevelResult)));
  const char* EncryptionKey = Config.EncryptionKey;
  if (!IsNullOrEmpty(EncryptionKey) &&
      std::strlen(EncryptionKey) != EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH) {
    M_LOG(
        "EOS EncryptionKey ignored: length is {}, expected {}.",
        std::strlen(EncryptionKey),
        EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH
    );
    EncryptionKey = nullptr;
  }
  EOS_Platform_Options PlatformOptions = {};
  PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
  PlatformOptions.ProductId = Config.ProductId;
  PlatformOptions.SandboxId = Config.SandboxId;
  PlatformOptions.DeploymentId = Config.DeploymentId;
  PlatformOptions.ClientCredentials.ClientId = Config.ClientId;
  PlatformOptions.ClientCredentials.ClientSecret = Config.ClientSecret;
  PlatformOptions.EncryptionKey = EncryptionKey;
  PlatformOptions.bIsServer = EOS_FALSE;
  PlatformHandle = EOS_Platform_Create(&PlatformOptions);
  if (!PlatformHandle) {
    M_LOG("EOS_Platform_Create failed.");
    EOS_Shutdown();
    bSDKInitialized = false;
    return false;
  }
  bInitialized = true;
  bTickLogged = false;
  TickCount = 0;
  M_LOG(
      "EOS_Platform_Create succeeded. PlatformHandle={}", static_cast<const void*>(PlatformHandle)
  );
  return true;
}

void EOSCoreManager::Tick() {
  if (!PlatformHandle) {
    return;
  }
  EOS_Platform_Tick(PlatformHandle);
  ++TickCount;
  if (!bTickLogged) {
    M_LOG("EOS_Platform_Tick started. Count={}", TickCount);
    bTickLogged = true;
  } else if (TickCount % 300 == 0) {
    M_LOG("EOS_Platform_Tick running. Count={}", TickCount);
  }
}

void EOSCoreManager::Shutdown() {
  if (TickCount > 0) {
    M_LOG("EOS_Platform_Tick total count: {}", TickCount);
  }
  if (PlatformHandle) {
    EOS_Platform_Release(PlatformHandle);
    PlatformHandle = nullptr;
    M_LOG("EOS_Platform_Release completed.");
  }
  if (bSDKInitialized) {
    EOS_EResult ShutdownResult = EOS_Shutdown();
    M_LOG("EOS_Shutdown result: {}", SafeEOSString(EOS_EResult_ToString(ShutdownResult)));
  }
  bInitialized = false;
  bSDKInitialized = false;
  bTickLogged = false;
  TickCount = 0;
}

bool EOSCoreManager::IsInitialized() const { return bInitialized; }
EOS_HPlatform EOSCoreManager::GetPlatformHandle() const { return PlatformHandle; }
EOS_HConnect EOSCoreManager::GetConnectHandle() const {
  if (!PlatformHandle) {
    return nullptr;
  }
  return EOS_Platform_GetConnectInterface(PlatformHandle);
}
EOS_HLobby EOSCoreManager::GetLobbyHandle() const {
  if (!PlatformHandle) {
    return nullptr;
  }
  return EOS_Platform_GetLobbyInterface(PlatformHandle);
}
