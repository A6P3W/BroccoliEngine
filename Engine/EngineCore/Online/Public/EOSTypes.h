#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct FEOSConfig {
  const char* ProductName = nullptr;
  const char* ProductVersion = nullptr;

  const char* ProductId = nullptr;
  const char* SandboxId = nullptr;
  const char* DeploymentId = nullptr;

  const char* ClientId = nullptr;
  const char* ClientSecret = nullptr;
  const char* EncryptionKey = nullptr;
};

enum class ELobbyAttributeType { String, Int64, Double, Bool };

enum class ELobbyDisconnectReason { Kicked, HostLeft, NetworkError, LobbyClosed, AuthLost };

enum class EAuthLossReason { TokenExpired, NetworkError, Unknown };

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
