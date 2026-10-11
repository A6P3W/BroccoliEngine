#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "NetworkTypes.h"
#ifdef EOS_PLUGIN_BUILD
#define EOS_PLUGIN_API __declspec(dllexport)
#else
#define EOS_PLUGIN_API __declspec(dllimport)
#endif

namespace BEPlugins::EOS {

enum class ELobbyAttributeType { String, Int64, Double, Bool };

struct FLobbyAttributeValue {
  ELobbyAttributeType Type = ELobbyAttributeType::String;

  std::string StringValue;
  int64_t IntValue = 0;
  double DoubleValue = 0.0;
  bool BoolValue = false;

  static FLobbyAttributeValue FromString(const std::string& Value) {
    FLobbyAttributeValue Result;
    Result.Type = ELobbyAttributeType::String;
    Result.StringValue = Value;
    return Result;
  }

  static FLobbyAttributeValue FromInt64(int64_t Value) {
    FLobbyAttributeValue Result;
    Result.Type = ELobbyAttributeType::Int64;
    Result.IntValue = Value;
    return Result;
  }

  static FLobbyAttributeValue FromDouble(double Value) {
    FLobbyAttributeValue Result;
    Result.Type = ELobbyAttributeType::Double;
    Result.DoubleValue = Value;
    return Result;
  }

  static FLobbyAttributeValue FromBool(bool Value) {
    FLobbyAttributeValue Result;
    Result.Type = ELobbyAttributeType::Bool;
    Result.BoolValue = Value;
    return Result;
  }
};

struct FLobbyAttribute {
  std::string Key;
  FLobbyAttributeValue Value;

  bool bAdvertised = true;
};

struct FLobbyInfo {
  std::string LobbyId;
  std::string OwnerProductUserId;
  std::string HostIPAddress;

  int CurrentMembers = 0;
  int MaxMembers = 0;

  std::vector<FLobbyAttribute> Attributes;

  bool bValid = false;

  std::string GetStringAttribute(
      const std::string& InKey, const std::string& DefaultValue = ""
  ) const {
    for (const auto& Attr : Attributes) {
      if (Attr.Key == InKey && Attr.Value.Type == ELobbyAttributeType::String) {
        return Attr.Value.StringValue;
      }
    }
    return DefaultValue;
  }
};

enum class ELobbyComparisonOp {
  Equal,
  NotEqual,
  GreaterThan,
  GreaterThanOrEqual,
  LessThan,
  LessThanOrEqual
};

struct FLobbySearchFilter {
  std::string Key;
  FLobbyAttributeValue Value;

  ELobbyComparisonOp ComparisonOp = ELobbyComparisonOp::Equal;
};

struct FCreateLobbyRequest {
  int MaxMembers = 4;
  bool bPublicAdvertised = true;
  std::string BucketId = "BroccoliNetworkTest";
  std::string HostIPAddress;
  uint16_t Port = 7777;

  std::vector<FLobbyAttribute> Attributes;
};

struct FLobbySearchRequest {
  int MaxResults = 10;
  std::string BucketId = "BroccoliNetworkTest";

  std::vector<FLobbySearchFilter> Filters;
};
enum class ETitleStorageDownloadError : uint8_t {
  None,
  EOSNotInitialized,
  NotLoggedIn,
  DownloadInProgress,
  InvalidFileName,
  DirectoryCreationFailed,
  TemporaryFileOpenFailed,
  DownloadStartFailed,
  DownloadFailed,
  FileWriteFailed,
  FinalFileDeleteFailed,
  FileRenameFailed,
  ArchiveExtractionFailed,
  ArchiveDeleteFailed
};

struct FTitleStorageDownloadResult {
  bool Success = false;
  ETitleStorageDownloadError Error = ETitleStorageDownloadError::DownloadFailed;
  std::string Message;
  std::string FileName;
  std::string LocalFilePath;
};

enum class EOnlinePlayState : uint8_t {
  Idle,
  LoggingIn,
  HostingLobby,
  SearchingLobbies,
  JoiningLobby,
  Hosting,
  Connected,
  Leaving
};
enum class EOnlinePlayError : uint8_t {
  None,
  EOSNotInitialized,
  NotLoggedIn,
  AlreadyLoggedIn,
  AlreadyInLobby,
  OperationInProgress,
  InvalidRequest,
  LocalAddressNotFound,
  LoginFailed,
  LobbyCreationFailed,
  LobbySearchFailed,
  LobbyJoinFailed,
  TransportStartFailed,
  TransportConnectFailed,
  NetModeApplyFailed,
  LobbyLeaveFailed,
  RollbackFailed,
  Unknown
};

struct FHostLobbyRequest {
  int MaxMembers = 4;
  bool PublicAdvertised = true;
  std::string BucketId = "BroccoliNetworkTest";
  uint16_t Port = 7777;
  std::vector<FLobbyAttribute> Attributes;
};

struct FOnlinePlayResult {
  bool Success = false;
  EOnlinePlayError Error = EOnlinePlayError::Unknown;
  std::string Message;
  FLobbyInfo LobbyInfo;
  std::string TransportName = "ENet";
  bool RollbackSucceeded = true;
};

using ResultCallback = std::function<void(const FOnlinePlayResult&)>;
using SearchCallback =
    std::function<void(const FOnlinePlayResult&, const std::vector<FLobbyInfo>&)>;
using CompletionCallback = std::function<void(const FTitleStorageDownloadResult&)>;
EOS_PLUGIN_API bool ConfigureTransport(std::string_view Name);
EOS_PLUGIN_API const std::string& GetTransportName();
EOS_PLUGIN_API bool Login(const char* DisplayName, ResultCallback OnComplete);
EOS_PLUGIN_API bool HostLobby(const FHostLobbyRequest& Request, ResultCallback OnComplete);
EOS_PLUGIN_API bool SearchLobbies(const FLobbySearchRequest& Request, SearchCallback OnComplete);
EOS_PLUGIN_API bool JoinLobby(
    const FLobbyInfo& LobbyInfo, uint16_t Port, ResultCallback OnComplete
);
EOS_PLUGIN_API bool LeaveLobby(ResultCallback OnComplete);
EOS_PLUGIN_API bool IsEOSInitialized();
EOS_PLUGIN_API bool IsLoggedIn();
EOS_PLUGIN_API bool IsInLobby();
EOS_PLUGIN_API bool IsOperationPending();
EOS_PLUGIN_API EOnlinePlayState GetState();
EOS_PLUGIN_API bool CanLogin();
EOS_PLUGIN_API bool CanHostLobby();
EOS_PLUGIN_API bool CanSearchLobbies();
EOS_PLUGIN_API bool CanJoinLobby();
EOS_PLUGIN_API bool CanLeaveLobby();
EOS_PLUGIN_API std::string GetLocalUserIdString();
EOS_PLUGIN_API std::string GetCurrentLobbyId();

EOS_PLUGIN_API bool Download(const std::string& FileName, CompletionCallback OnComplete);
EOS_PLUGIN_API bool IsDownloadPending();
EOS_PLUGIN_API std::string GetDownloadDirectoryPath();
}  // namespace BEPlugins::EOS
