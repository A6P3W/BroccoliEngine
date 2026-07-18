#include "EOSLobbyManager.h"

#include <eos_lobby.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "Log.h"

namespace {
const char* HostIPAttributeKey = "HostIP";

const char* SafeText(const char* Text) { return Text ? Text : "<null>"; }

const char* SafeEOSResult(EOS_EResult Result) { return SafeText(EOS_EResult_ToString(Result)); }

const char* GetSafeBucketId(const FCreateLobbyRequest& Request) {
  return Request.BucketId.empty() ? "BroccoliNetworkTest" : Request.BucketId.c_str();
}

const char* GetSafeBucketId(const FLobbySearchRequest& Request) {
  return Request.BucketId.empty() ? "BroccoliNetworkTest" : Request.BucketId.c_str();
}

int ClampToInt(uint32_t Value) {
  return Value > static_cast<uint32_t>(INT32_MAX) ? INT32_MAX : static_cast<int>(Value);
}

std::string ProductUserIdToString(EOS_ProductUserId UserId) {
  if (!UserId) {
    return {};
  }

  std::array<char, EOS_PRODUCTUSERID_MAX_LENGTH + 1> Buffer = {};
  int32_t BufferLength = static_cast<int32_t>(Buffer.size());
  EOS_EResult Result = EOS_ProductUserId_ToString(UserId, Buffer.data(), &BufferLength);
  if (Result != EOS_EResult::EOS_Success) {
    M_LOG("[EOSLobby] ProductUserId stringify failed: {}", SafeEOSResult(Result));
    return {};
  }

  return std::string(Buffer.data());
}

bool IsSameProductUserId(EOS_ProductUserId A, EOS_ProductUserId B) {
  if (!A || !B) {
    return false;
  }

  return ProductUserIdToString(A) == ProductUserIdToString(B);
}

const char* LobbyDisconnectReasonToString(ELobbyDisconnectReason Reason) {
  switch (Reason) {
    case ELobbyDisconnectReason::Kicked:
      return "Kicked";
    case ELobbyDisconnectReason::HostLeft:
      return "HostLeft";
    case ELobbyDisconnectReason::NetworkError:
      return "NetworkError";
    case ELobbyDisconnectReason::LobbyClosed:
      return "LobbyClosed";
    case ELobbyDisconnectReason::AuthLost:
      return "AuthLost";
    default:
      return "Unknown";
  }
}

const char* LobbyMemberStatusToString(EOS_ELobbyMemberStatus Status) {
  switch (Status) {
    case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
      return "Joined";
    case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
      return "Left";
    case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
      return "Disconnected";
    case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
      return "Kicked";
    case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
      return "Promoted";
    case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
      return "Closed";
    default:
      return "Unknown";
  }
}

bool TryGetDisconnectReasonFromMemberStatus(
    EOS_ELobbyMemberStatus Status, ELobbyDisconnectReason& OutReason
) {
  switch (Status) {
    case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
      OutReason = ELobbyDisconnectReason::HostLeft;
      return true;
    case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
      OutReason = ELobbyDisconnectReason::Kicked;
      return true;
    case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
      OutReason = ELobbyDisconnectReason::NetworkError;
      return true;
    default:
      return false;
  }
}

EOS_ELobbyAttributeType ToEOSAttributeType(ELobbyAttributeType Type) {
  switch (Type) {
    case ELobbyAttributeType::Int64:
      return EOS_ELobbyAttributeType::EOS_AT_INT64;
    case ELobbyAttributeType::Double:
      return EOS_ELobbyAttributeType::EOS_AT_DOUBLE;
    case ELobbyAttributeType::Bool:
      return EOS_ELobbyAttributeType::EOS_AT_BOOLEAN;
    case ELobbyAttributeType::String:
    default:
      return EOS_ELobbyAttributeType::EOS_AT_STRING;
  }
}

EOS_EComparisonOp ToEOSComparisonOp(ELobbyComparisonOp Op) {
  switch (Op) {
    case ELobbyComparisonOp::NotEqual:
      return EOS_EComparisonOp::EOS_CO_NOTEQUAL;
    case ELobbyComparisonOp::GreaterThan:
      return EOS_EComparisonOp::EOS_CO_GREATERTHAN;
    case ELobbyComparisonOp::GreaterThanOrEqual:
      return EOS_EComparisonOp::EOS_CO_GREATERTHANOREQUAL;
    case ELobbyComparisonOp::LessThan:
      return EOS_EComparisonOp::EOS_CO_LESSTHAN;
    case ELobbyComparisonOp::LessThanOrEqual:
      return EOS_EComparisonOp::EOS_CO_LESSTHANOREQUAL;
    case ELobbyComparisonOp::Equal:
    default:
      return EOS_EComparisonOp::EOS_CO_EQUAL;
  }
}

void FillEOSAttributeData(
    const std::string& Key, const FLobbyAttributeValue& Value, EOS_Lobby_AttributeData& OutData
) {
  OutData = {};
  OutData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
  OutData.Key = Key.c_str();
  OutData.ValueType = ToEOSAttributeType(Value.Type);

  switch (Value.Type) {
    case ELobbyAttributeType::Int64:
      OutData.Value.AsInt64 = Value.IntValue;
      break;
    case ELobbyAttributeType::Double:
      OutData.Value.AsDouble = Value.DoubleValue;
      break;
    case ELobbyAttributeType::Bool:
      OutData.Value.AsBool = Value.BoolValue ? EOS_TRUE : EOS_FALSE;
      break;
    case ELobbyAttributeType::String:
    default:
      OutData.Value.AsUtf8 = Value.StringValue.c_str();
      break;
  }
}

FLobbyAttributeValue MakeLobbyAttributeValue(const EOS_Lobby_AttributeData& Data) {
  switch (Data.ValueType) {
    case EOS_ELobbyAttributeType::EOS_AT_INT64:
      return FLobbyAttributeValue::FromInt64(Data.Value.AsInt64);
    case EOS_ELobbyAttributeType::EOS_AT_DOUBLE:
      return FLobbyAttributeValue::FromDouble(Data.Value.AsDouble);
    case EOS_ELobbyAttributeType::EOS_AT_BOOLEAN:
      return FLobbyAttributeValue::FromBool(Data.Value.AsBool == EOS_TRUE);
    case EOS_ELobbyAttributeType::EOS_AT_STRING:
    default:
      return FLobbyAttributeValue::FromString(Data.Value.AsUtf8 ? Data.Value.AsUtf8 : "");
  }
}

bool AddLobbyAttributeToModification(
    EOS_HLobbyModification ModificationHandle,
    const FLobbyAttribute& Attribute,
    EOS_Lobby_AttributeData& AttributeData
) {
  if (!ModificationHandle || Attribute.Key.empty()) {
    M_LOG("[EOSLobby] Add lobby attribute skipped: invalid handle or empty key.");
    return false;
  }

  FillEOSAttributeData(Attribute.Key, Attribute.Value, AttributeData);

  EOS_LobbyModification_AddAttributeOptions AddAttributeOptions = {};
  AddAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
  AddAttributeOptions.Attribute = &AttributeData;
  AddAttributeOptions.Visibility = Attribute.bAdvertised
                                       ? EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC
                                       : EOS_ELobbyAttributeVisibility::EOS_LAT_PRIVATE;

  EOS_EResult AddAttributeResult =
      EOS_LobbyModification_AddAttribute(ModificationHandle, &AddAttributeOptions);
  if (AddAttributeResult != EOS_EResult::EOS_Success) {
    M_LOG(
        "[EOSLobby] Add lobby attribute failed: key={}, result={}",
        Attribute.Key,
        SafeEOSResult(AddAttributeResult)
    );
    return false;
  }

  return true;
}

bool SetLobbySearchFilter(EOS_HLobbySearch SearchHandle, const FLobbySearchFilter& Filter) {
  if (!SearchHandle || Filter.Key.empty()) {
    M_LOG("[EOSLobby] Set lobby search filter skipped: invalid handle or empty key.");
    return false;
  }

  EOS_Lobby_AttributeData Parameter = {};
  FillEOSAttributeData(Filter.Key, Filter.Value, Parameter);

  EOS_LobbySearch_SetParameterOptions SetParameterOptions = {};
  SetParameterOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
  SetParameterOptions.Parameter = &Parameter;
  SetParameterOptions.ComparisonOp = ToEOSComparisonOp(Filter.ComparisonOp);

  EOS_EResult SetParameterResult = EOS_LobbySearch_SetParameter(SearchHandle, &SetParameterOptions);
  if (SetParameterResult != EOS_EResult::EOS_Success) {
    M_LOG(
        "[EOSLobby] SearchLobbies set filter failed: key={}, result={}",
        Filter.Key,
        SafeEOSResult(SetParameterResult)
    );
    return false;
  }

  return true;
}

std::vector<FLobbyAttribute> CopyLobbyAttributes(EOS_HLobbyDetails DetailsHandle) {
  std::vector<FLobbyAttribute> Attributes;
  if (!DetailsHandle) {
    return Attributes;
  }

  EOS_LobbyDetails_GetAttributeCountOptions CountOptions = {};
  CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
  const uint32_t AttributeCount = EOS_LobbyDetails_GetAttributeCount(DetailsHandle, &CountOptions);
  Attributes.reserve(AttributeCount);

  for (uint32_t Index = 0; Index < AttributeCount; ++Index) {
    EOS_LobbyDetails_CopyAttributeByIndexOptions CopyOptions = {};
    CopyOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
    CopyOptions.AttrIndex = Index;

    EOS_Lobby_Attribute* Attribute = nullptr;
    EOS_EResult CopyResult =
        EOS_LobbyDetails_CopyAttributeByIndex(DetailsHandle, &CopyOptions, &Attribute);
    if (CopyResult != EOS_EResult::EOS_Success || !Attribute || !Attribute->Data) {
      M_LOG(
          "[EOSLobby] Copy lobby attribute by index failed: index={}, result={}",
          Index,
          SafeEOSResult(CopyResult)
      );
      if (Attribute) {
        EOS_Lobby_Attribute_Release(Attribute);
      }
      continue;
    }

    FLobbyAttribute LobbyAttribute;
    LobbyAttribute.Key = Attribute->Data->Key ? Attribute->Data->Key : "";
    LobbyAttribute.Value = MakeLobbyAttributeValue(*Attribute->Data);
    LobbyAttribute.bAdvertised =
        Attribute->Visibility == EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
    Attributes.push_back(LobbyAttribute);

    EOS_Lobby_Attribute_Release(Attribute);
  }

  return Attributes;
}

FLobbyAttribute MakeHostIPAttribute(const std::string& HostIPAddress) {
  FLobbyAttribute Attribute;
  Attribute.Key = HostIPAttributeKey;
  Attribute.Value = FLobbyAttributeValue::FromString(HostIPAddress);
  Attribute.bAdvertised = true;
  return Attribute;
}
EOS_ProductUserId GetLoggedInUserId() {
  EOSAuthManager& AuthManager = EOSAuthManager::Get();
  return AuthManager.IsLoggedIn() ? AuthManager.GetLocalUserId() : nullptr;
}

std::string CopyStringLobbyAttributeByKey(EOS_HLobbyDetails DetailsHandle, const char* Key) {
  if (!DetailsHandle || !Key) {
    return {};
  }

  EOS_LobbyDetails_CopyAttributeByKeyOptions CopyAttributeOptions = {};
  CopyAttributeOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYKEY_API_LATEST;
  CopyAttributeOptions.AttrKey = Key;

  EOS_Lobby_Attribute* Attribute = nullptr;
  EOS_EResult CopyResult =
      EOS_LobbyDetails_CopyAttributeByKey(DetailsHandle, &CopyAttributeOptions, &Attribute);
  if (CopyResult != EOS_EResult::EOS_Success || !Attribute || !Attribute->Data) {
    if (CopyResult != EOS_EResult::EOS_NotFound) {
      M_LOG(
          "[EOSLobby] Copy lobby attribute failed: key={}, result={}",
          Key,
          SafeEOSResult(CopyResult)
      );
    }
    if (Attribute) {
      EOS_Lobby_Attribute_Release(Attribute);
    }
    return {};
  }

  std::string Value;
  if (Attribute->Data->ValueType == EOS_ELobbyAttributeType::EOS_AT_STRING &&
      Attribute->Data->Value.AsUtf8) {
    Value = Attribute->Data->Value.AsUtf8;
  }

  EOS_Lobby_Attribute_Release(Attribute);
  return Value;
}

FLobbyInfo MakeLobbyInfoFromDetails(EOS_HLobbyDetails DetailsHandle) {
  FLobbyInfo LobbyInfo;
  if (!DetailsHandle) {
    return LobbyInfo;
  }

  EOS_LobbyDetails_CopyInfoOptions CopyInfoOptions = {};
  CopyInfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;

  EOS_LobbyDetails_Info* DetailsInfo = nullptr;
  EOS_EResult CopyResult = EOS_LobbyDetails_CopyInfo(DetailsHandle, &CopyInfoOptions, &DetailsInfo);
  if (CopyResult != EOS_EResult::EOS_Success || !DetailsInfo) {
    M_LOG("[EOSLobby] Copy lobby details failed: {}", SafeEOSResult(CopyResult));
    return LobbyInfo;
  }

  LobbyInfo.LobbyId = DetailsInfo->LobbyId ? DetailsInfo->LobbyId : "";
  LobbyInfo.OwnerProductUserId = ProductUserIdToString(DetailsInfo->LobbyOwnerUserId);
  LobbyInfo.MaxMembers = ClampToInt(DetailsInfo->MaxMembers);
  LobbyInfo.CurrentMembers = ClampToInt(
      DetailsInfo->MaxMembers - (std::min)(DetailsInfo->AvailableSlots, DetailsInfo->MaxMembers)
  );
  LobbyInfo.bValid = !LobbyInfo.LobbyId.empty();
  LobbyInfo.Attributes = CopyLobbyAttributes(DetailsHandle);
  for (const FLobbyAttribute& Attribute : LobbyInfo.Attributes) {
    if (Attribute.Key == HostIPAttributeKey &&
        Attribute.Value.Type == ELobbyAttributeType::String) {
      LobbyInfo.HostIPAddress = Attribute.Value.StringValue;
      break;
    }
  }
  if (LobbyInfo.HostIPAddress.empty()) {
    LobbyInfo.HostIPAddress = CopyStringLobbyAttributeByKey(DetailsHandle, HostIPAttributeKey);
  }

  EOS_LobbyDetails_Info_Release(DetailsInfo);
  return LobbyInfo;
}

struct FCreateLobbyContext {
  FCreateLobbyRequest Request;
  FLobbyInfo LobbyInfo;
  EOS_HLobbyModification ModificationHandle = nullptr;
  std::vector<EOS_Lobby_AttributeData> AttributeDataStorage;
  std::function<void(bool, const FLobbyInfo&)> OnComplete;
};

struct FUpdateLobbyAttributesContext {
  EOS_HLobbyModification ModificationHandle = nullptr;
  std::vector<EOS_Lobby_AttributeData> AttributeDataStorage;
  std::function<void(bool)> OnComplete;
};

struct FLeaveLobbyContext {
  std::string LobbyId;
  std::function<void(bool)> OnComplete;
};

struct FSearchLobbiesContext {
  EOS_HLobbySearch SearchHandle = nullptr;
  std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete;
};

struct FFetchLobbyInfoContext {
  EOS_HLobbySearch SearchHandle = nullptr;
  std::string LobbyId;
  std::function<void(bool, const FLobbyInfo&)> OnComplete;
};

struct FJoinLobbyContext {
  std::string LobbyId;
  std::function<void(bool)> OnComplete;
};

void CompleteCreateCallback(
    std::function<void(bool, const FLobbyInfo&)>& OnComplete,
    bool bSuccess,
    const FLobbyInfo& LobbyInfo
) {
  if (OnComplete) {
    OnComplete(bSuccess, LobbyInfo);
  }
}

void CompleteBoolCallback(std::function<void(bool)>& OnComplete, bool bSuccess) {
  if (OnComplete) {
    OnComplete(bSuccess);
  }
}

void CompleteSearchCallback(
    std::function<void(bool, const std::vector<FLobbyInfo>&)>& OnComplete,
    bool bSuccess,
    const std::vector<FLobbyInfo>& Results
) {
  if (OnComplete) {
    OnComplete(bSuccess, Results);
  }
}

void CompleteFetchCallback(
    std::function<void(bool, const FLobbyInfo&)>& OnComplete,
    bool bSuccess,
    const FLobbyInfo& LobbyInfo
) {
  if (OnComplete) {
    OnComplete(bSuccess, LobbyInfo);
  }
}
}  // namespace

struct FCachedLobbyDetails {
  std::string LobbyId;
  EOS_HLobbyDetails DetailsHandle = nullptr;
};

struct EOSLobbyManager::Impl {
  std::string CurrentLobbyId;
  bool bInLobby = false;
  std::vector<FCachedLobbyDetails> CachedLobbyDetails;
  EOS_NotificationId MemberStatusNotificationId = EOS_INVALID_NOTIFICATIONID;
  EOS_NotificationId LobbyUpdateNotificationId = EOS_INVALID_NOTIFICATIONID;
  std::function<void(ELobbyDisconnectReason)> OnLobbyDisconnected;
};

EOSLobbyManager& EOSLobbyManager::Get() {
  static EOSLobbyManager Instance;
  return Instance;
}

EOSLobbyManager::EOSLobbyManager() : ImplPtr(new Impl()) {}

EOSLobbyManager::~EOSLobbyManager() { delete ImplPtr; }

void EOSLobbyManager::Shutdown() {
  UnregisterLobbyNotifications();
  ClearCachedLobbyDetails();
  ImplPtr->OnLobbyDisconnected = nullptr;
  ImplPtr->CurrentLobbyId.clear();
  ImplPtr->bInLobby = false;
}

void EOSLobbyManager::CreateLobby(
    const FCreateLobbyRequest& Request, std::function<void(bool, const FLobbyInfo&)> OnComplete
) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!EOSCoreManager::Get().IsInitialized() || !LobbyHandle || !LocalUserId) {
    M_LOG("[EOSLobby] CreateLobby failed: EOS is not initialized or user is not logged in.");
    CompleteCreateCallback(OnComplete, false, {});
    return;
  }

  EOS_Lobby_CreateLobbyOptions Options = {};
  Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
  Options.LocalUserId = LocalUserId;
  Options.MaxLobbyMembers = static_cast<uint32_t>((std::max)(1, Request.MaxMembers));
  Options.PermissionLevel = Request.bPublicAdvertised
                                ? EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED
                                : EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
  Options.bPresenceEnabled = EOS_FALSE;
  Options.bAllowInvites = EOS_TRUE;
  Options.BucketId = GetSafeBucketId(Request);
  Options.bDisableHostMigration = EOS_TRUE;
  Options.bEnableRTCRoom = EOS_FALSE;
  Options.LocalRTCOptions = nullptr;
  Options.LobbyId = nullptr;
  Options.bEnableJoinById = EOS_FALSE;
  Options.bRejoinAfterKickRequiresInvite = EOS_FALSE;
  Options.AllowedPlatformIds = nullptr;
  Options.AllowedPlatformIdsCount = 0;
  Options.bCrossplayOptOut = EOS_FALSE;
  Options.RTCRoomJoinActionType = EOS_ELobbyRTCRoomJoinActionType::EOS_LRRJAT_ManualJoin;

  auto* Context = new FCreateLobbyContext{Request, {}, nullptr, {}, std::move(OnComplete)};
  EOS_Lobby_CreateLobby(
      LobbyHandle, &Options, Context, [](const EOS_Lobby_CreateLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FCreateLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] CreateLobby failed: callback data is null.");
          LobbyManager.ImplPtr->CurrentLobbyId.clear();
          LobbyManager.ImplPtr->bInLobby = false;
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success && Data->LobbyId) {
          FLobbyInfo LobbyInfo;
          LobbyInfo.LobbyId = Data->LobbyId;
          LobbyInfo.OwnerProductUserId = ProductUserIdToString(GetLoggedInUserId());
          LobbyInfo.HostIPAddress = Context->Request.HostIPAddress;
          LobbyInfo.CurrentMembers = 1;
          LobbyInfo.MaxMembers = (std::max)(1, Context->Request.MaxMembers);
          LobbyInfo.bValid = true;

          LobbyManager.ImplPtr->CurrentLobbyId = LobbyInfo.LobbyId;
          LobbyManager.ImplPtr->bInLobby = true;
          LobbyManager.RegisterLobbyNotifications();

          M_LOG("[EOSLobby] CreateLobby success");
          M_LOG("[EOSLobby] LobbyId = {}", LobbyManager.ImplPtr->CurrentLobbyId);

          LobbyInfo.Attributes = Context->Request.Attributes;
          if (!Context->Request.HostIPAddress.empty()) {
            LobbyInfo.Attributes.push_back(MakeHostIPAttribute(Context->Request.HostIPAddress));
          }

          if (LobbyInfo.Attributes.empty()) {
            CompleteCreateCallback(Context->OnComplete, true, LobbyInfo);
            delete Context;
            return;
          }

          EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
          EOS_ProductUserId LocalUserId = GetLoggedInUserId();
          if (!LobbyHandle || !LocalUserId) {
            M_LOG("[EOSLobby] Add attributes failed: EOS lobby handle or local user is invalid.");
            CompleteCreateCallback(Context->OnComplete, false, LobbyInfo);
            delete Context;
            return;
          }

          EOS_HLobbyModification ModificationHandle = nullptr;
          EOS_Lobby_UpdateLobbyModificationOptions ModificationOptions = {};
          ModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
          ModificationOptions.LocalUserId = LocalUserId;
          ModificationOptions.LobbyId = LobbyInfo.LobbyId.c_str();

          EOS_EResult ModificationResult = EOS_Lobby_UpdateLobbyModification(
              LobbyHandle, &ModificationOptions, &ModificationHandle
          );
          if (ModificationResult != EOS_EResult::EOS_Success || !ModificationHandle) {
            M_LOG(
                "[EOSLobby] UpdateLobbyModification failed: {}", SafeEOSResult(ModificationResult)
            );
            CompleteCreateCallback(Context->OnComplete, false, LobbyInfo);
            delete Context;
            return;
          }

          Context->AttributeDataStorage.resize(LobbyInfo.Attributes.size());
          for (size_t AttributeIndex = 0; AttributeIndex < LobbyInfo.Attributes.size();
               ++AttributeIndex) {
            const FLobbyAttribute& Attribute = LobbyInfo.Attributes[AttributeIndex];
            if (!AddLobbyAttributeToModification(
                    ModificationHandle, Attribute, Context->AttributeDataStorage[AttributeIndex]
                )) {
              EOS_LobbyModification_Release(ModificationHandle);
              CompleteCreateCallback(Context->OnComplete, false, LobbyInfo);
              delete Context;
              return;
            }
          }

          Context->LobbyInfo = LobbyInfo;
          Context->ModificationHandle = ModificationHandle;

          EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
          UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
          UpdateOptions.LobbyModificationHandle = ModificationHandle;
          EOS_Lobby_UpdateLobby(
              LobbyHandle,
              &UpdateOptions,
              Context,
              [](const EOS_Lobby_UpdateLobbyCallbackInfo* UpdateData) {
                auto* Context = static_cast<FCreateLobbyContext*>(
                    UpdateData ? UpdateData->ClientData : nullptr
                );
                if (!UpdateData || !Context) {
                  M_LOG("[EOSLobby] UpdateLobby failed: callback data is null.");
                  return;
                }

                if (UpdateData->ResultCode == EOS_EResult::EOS_Success) {
                  M_LOG(
                      "[EOSLobby] Lobby attributes updated: count={}",
                      Context->LobbyInfo.Attributes.size()
                  );
                  CompleteCreateCallback(Context->OnComplete, true, Context->LobbyInfo);
                } else {
                  M_LOG("[EOSLobby] UpdateLobby failed: {}", SafeEOSResult(UpdateData->ResultCode));
                  CompleteCreateCallback(Context->OnComplete, false, Context->LobbyInfo);
                }
                if (Context->ModificationHandle) {
                  EOS_LobbyModification_Release(Context->ModificationHandle);
                  Context->ModificationHandle = nullptr;
                }
                delete Context;
              }
          );
          return;
        }

        M_LOG("[EOSLobby] CreateLobby failed: {}", SafeEOSResult(Data->ResultCode));
        LobbyManager.ImplPtr->CurrentLobbyId.clear();
        LobbyManager.ImplPtr->bInLobby = false;
        CompleteCreateCallback(Context->OnComplete, false, {});
        delete Context;
      }
  );
}

void EOSLobbyManager::UpdateCurrentLobbyAttributes(
    const std::vector<FLobbyAttribute>& Attributes, std::function<void(bool)> OnComplete
) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!LobbyHandle || !LocalUserId || !ImplPtr->bInLobby || ImplPtr->CurrentLobbyId.empty()) {
    M_LOG("[EOSLobby] UpdateCurrentLobbyAttributes failed: current lobby state is invalid.");
    CompleteBoolCallback(OnComplete, false);
    return;
  }

  EOS_HLobbyModification ModificationHandle = nullptr;
  EOS_Lobby_UpdateLobbyModificationOptions ModificationOptions = {};
  ModificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
  ModificationOptions.LocalUserId = LocalUserId;
  ModificationOptions.LobbyId = ImplPtr->CurrentLobbyId.c_str();

  EOS_EResult ModificationResult =
      EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModificationOptions, &ModificationHandle);
  if (ModificationResult != EOS_EResult::EOS_Success || !ModificationHandle) {
    M_LOG("[EOSLobby] UpdateLobbyModification failed: {}", SafeEOSResult(ModificationResult));
    CompleteBoolCallback(OnComplete, false);
    return;
  }

  auto* Context = new FUpdateLobbyAttributesContext{};
  Context->ModificationHandle = ModificationHandle;
  Context->AttributeDataStorage.resize(Attributes.size());
  Context->OnComplete = std::move(OnComplete);

  for (size_t AttributeIndex = 0; AttributeIndex < Attributes.size(); ++AttributeIndex) {
    if (!AddLobbyAttributeToModification(
            ModificationHandle,
            Attributes[AttributeIndex],
            Context->AttributeDataStorage[AttributeIndex]
        )) {
      EOS_LobbyModification_Release(ModificationHandle);
      Context->ModificationHandle = nullptr;
      CompleteBoolCallback(Context->OnComplete, false);
      delete Context;
      return;
    }
  }

  EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
  UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
  UpdateOptions.LobbyModificationHandle = ModificationHandle;
  EOS_Lobby_UpdateLobby(
      LobbyHandle,
      &UpdateOptions,
      Context,
      [](const EOS_Lobby_UpdateLobbyCallbackInfo* UpdateData) {
        auto* Context = static_cast<FUpdateLobbyAttributesContext*>(
            UpdateData ? UpdateData->ClientData : nullptr
        );
        if (!UpdateData || !Context) {
          M_LOG("[EOSLobby] UpdateCurrentLobbyAttributes failed: callback data is null.");
          return;
        }

        const bool bSuccess = UpdateData->ResultCode == EOS_EResult::EOS_Success;
        if (!bSuccess) {
          M_LOG(
              "[EOSLobby] UpdateCurrentLobbyAttributes failed: {}",
              SafeEOSResult(UpdateData->ResultCode)
          );
        }
        if (Context->ModificationHandle) {
          EOS_LobbyModification_Release(Context->ModificationHandle);
          Context->ModificationHandle = nullptr;
        }
        CompleteBoolCallback(Context->OnComplete, bSuccess);
        delete Context;
      }
  );
}

void EOSLobbyManager::LeaveLobby(std::function<void(bool)> OnComplete) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!LobbyHandle || !LocalUserId || !ImplPtr->bInLobby || ImplPtr->CurrentLobbyId.empty()) {
    M_LOG("[EOSLobby] LeaveLobby failed: current lobby state is invalid.");
    CompleteBoolCallback(OnComplete, false);
    return;
  }

  auto* Context = new FLeaveLobbyContext{ImplPtr->CurrentLobbyId, std::move(OnComplete)};
  UnregisterLobbyNotifications();

  EOS_Lobby_LeaveLobbyOptions Options = {};
  Options.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
  Options.LocalUserId = LocalUserId;
  Options.LobbyId = Context->LobbyId.c_str();
  EOS_Lobby_LeaveLobby(
      LobbyHandle, &Options, Context, [](const EOS_Lobby_LeaveLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FLeaveLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] LeaveLobby failed: callback data is null.");
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success ||
            Data->ResultCode == EOS_EResult::EOS_NotFound) {
          LobbyManager.ImplPtr->CurrentLobbyId.clear();
          LobbyManager.ImplPtr->bInLobby = false;
          LobbyManager.ClearCachedLobbyDetails();
          if (Data->ResultCode == EOS_EResult::EOS_Success) {
            M_LOG("[EOSLobby] LeaveLobby success");
          } else {
            M_LOG("[EOSLobby] LeaveLobby recovered: lobby was already gone.");
          }
          CompleteBoolCallback(Context->OnComplete, true);
          delete Context;
          return;
        }

        M_LOG("[EOSLobby] LeaveLobby failed: {}", SafeEOSResult(Data->ResultCode));
        LobbyManager.RegisterLobbyNotifications();
        CompleteBoolCallback(Context->OnComplete, false);
        delete Context;
      }
  );
}

void EOSLobbyManager::SearchLobbies(
    const FLobbySearchRequest& Request,
    std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete
) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!EOSCoreManager::Get().IsInitialized() || !LobbyHandle || !LocalUserId) {
    M_LOG("[EOSLobby] SearchLobbies failed: EOS is not initialized or user is not logged in.");
    CompleteSearchCallback(OnComplete, false, {});
    return;
  }

  ClearCachedLobbyDetails();

  EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions = {};
  CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
  CreateSearchOptions.MaxResults = static_cast<uint32_t>((std::max)(1, Request.MaxResults));

  EOS_HLobbySearch SearchHandle = nullptr;
  EOS_EResult CreateSearchResult =
      EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &SearchHandle);
  if (CreateSearchResult != EOS_EResult::EOS_Success || !SearchHandle) {
    M_LOG("[EOSLobby] CreateLobbySearch failed: {}", SafeEOSResult(CreateSearchResult));
    CompleteSearchCallback(OnComplete, false, {});
    return;
  }

  EOS_Lobby_AttributeData BucketParameter = {};
  BucketParameter.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
  BucketParameter.Key = EOS_LOBBY_SEARCH_BUCKET_ID;
  BucketParameter.Value.AsUtf8 = GetSafeBucketId(Request);
  BucketParameter.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;

  EOS_LobbySearch_SetParameterOptions SetBucketOptions = {};
  SetBucketOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
  SetBucketOptions.Parameter = &BucketParameter;
  SetBucketOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
  EOS_EResult SetBucketResult = EOS_LobbySearch_SetParameter(SearchHandle, &SetBucketOptions);
  if (SetBucketResult != EOS_EResult::EOS_Success) {
    M_LOG("[EOSLobby] SearchLobbies set bucket failed: {}", SafeEOSResult(SetBucketResult));
    EOS_LobbySearch_Release(SearchHandle);
    CompleteSearchCallback(OnComplete, false, {});
    return;
  }

  for (const FLobbySearchFilter& Filter : Request.Filters) {
    if (!SetLobbySearchFilter(SearchHandle, Filter)) {
      EOS_LobbySearch_Release(SearchHandle);
      CompleteSearchCallback(OnComplete, false, {});
      return;
    }
  }
  EOS_LobbySearch_FindOptions FindOptions = {};
  FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
  FindOptions.LocalUserId = LocalUserId;

  auto* Context = new FSearchLobbiesContext{SearchHandle, std::move(OnComplete)};
  EOS_LobbySearch_Find(
      SearchHandle, &FindOptions, Context, [](const EOS_LobbySearch_FindCallbackInfo* Data) {
        auto* Context = static_cast<FSearchLobbiesContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();
        std::vector<FLobbyInfo> Results;

        if (!Data || !Context) {
          M_LOG("[EOSLobby] SearchLobbies failed: callback data is null.");
          return;
        }

        if (Data->ResultCode != EOS_EResult::EOS_Success) {
          M_LOG("[EOSLobby] SearchLobbies failed: {}", SafeEOSResult(Data->ResultCode));
          EOS_LobbySearch_Release(Context->SearchHandle);
          CompleteSearchCallback(Context->OnComplete, false, Results);
          delete Context;
          return;
        }

        EOS_LobbySearch_GetSearchResultCountOptions CountOptions = {};
        CountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
        uint32_t ResultCount =
            EOS_LobbySearch_GetSearchResultCount(Context->SearchHandle, &CountOptions);

        Results.reserve(ResultCount);
        LobbyManager.ImplPtr->CachedLobbyDetails.reserve(ResultCount);

        for (uint32_t Index = 0; Index < ResultCount; ++Index) {
          EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
          CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
          CopyOptions.LobbyIndex = Index;

          EOS_HLobbyDetails DetailsHandle = nullptr;
          EOS_EResult CopyResult = EOS_LobbySearch_CopySearchResultByIndex(
              Context->SearchHandle, &CopyOptions, &DetailsHandle
          );
          if (CopyResult != EOS_EResult::EOS_Success || !DetailsHandle) {
            M_LOG(
                "[EOSLobby] CopySearchResultByIndex failed: index={}, result={}",
                Index,
                SafeEOSResult(CopyResult)
            );
            continue;
          }

          FLobbyInfo LobbyInfo = MakeLobbyInfoFromDetails(DetailsHandle);
          if (!LobbyInfo.bValid) {
            EOS_LobbyDetails_Release(DetailsHandle);
            continue;
          }

          Results.push_back(LobbyInfo);
          LobbyManager.ImplPtr->CachedLobbyDetails.push_back({LobbyInfo.LobbyId, DetailsHandle});
        }

        EOS_LobbySearch_Release(Context->SearchHandle);
        M_LOG("[EOSLobby] SearchLobbies success");
        M_LOG("[EOSLobby] Found lobby count = {}", Results.size());
        CompleteSearchCallback(Context->OnComplete, true, Results);
        delete Context;
      }
  );
}

void EOSLobbyManager::FetchLobbyInfoById(
    const std::string& LobbyId, std::function<void(bool, const FLobbyInfo&)> OnComplete
) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!EOSCoreManager::Get().IsInitialized() || !LobbyHandle || !LocalUserId || LobbyId.empty()) {
    M_LOG("[EOSLobby] FetchLobbyInfoById failed: EOS state or lobby id is invalid.");
    CompleteFetchCallback(OnComplete, false, {});
    return;
  }

  EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions = {};
  CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
  CreateSearchOptions.MaxResults = 1;

  EOS_HLobbySearch SearchHandle = nullptr;
  EOS_EResult CreateSearchResult =
      EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &SearchHandle);
  if (CreateSearchResult != EOS_EResult::EOS_Success || !SearchHandle) {
    M_LOG(
        "[EOSLobby] FetchLobbyInfoById CreateLobbySearch failed: {}",
        SafeEOSResult(CreateSearchResult)
    );
    CompleteFetchCallback(OnComplete, false, {});
    return;
  }

  EOS_LobbySearch_SetLobbyIdOptions SetLobbyIdOptions = {};
  SetLobbyIdOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
  SetLobbyIdOptions.LobbyId = LobbyId.c_str();

  EOS_EResult SetLobbyIdResult = EOS_LobbySearch_SetLobbyId(SearchHandle, &SetLobbyIdOptions);
  if (SetLobbyIdResult != EOS_EResult::EOS_Success) {
    M_LOG("[EOSLobby] FetchLobbyInfoById SetLobbyId failed: {}", SafeEOSResult(SetLobbyIdResult));
    EOS_LobbySearch_Release(SearchHandle);
    CompleteFetchCallback(OnComplete, false, {});
    return;
  }

  EOS_LobbySearch_FindOptions FindOptions = {};
  FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
  FindOptions.LocalUserId = LocalUserId;

  auto* Context = new FFetchLobbyInfoContext{SearchHandle, LobbyId, std::move(OnComplete)};
  EOS_LobbySearch_Find(
      SearchHandle, &FindOptions, Context, [](const EOS_LobbySearch_FindCallbackInfo* Data) {
        auto* Context = static_cast<FFetchLobbyInfoContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] FetchLobbyInfoById failed: callback data is null.");
          return;
        }

        if (Data->ResultCode != EOS_EResult::EOS_Success) {
          M_LOG("[EOSLobby] FetchLobbyInfoById failed: {}", SafeEOSResult(Data->ResultCode));
          EOS_LobbySearch_Release(Context->SearchHandle);
          CompleteFetchCallback(Context->OnComplete, false, {});
          delete Context;
          return;
        }

        EOS_LobbySearch_GetSearchResultCountOptions CountOptions = {};
        CountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
        const uint32_t ResultCount =
            EOS_LobbySearch_GetSearchResultCount(Context->SearchHandle, &CountOptions);
        if (ResultCount == 0) {
          M_LOG(
              "[EOSLobby] FetchLobbyInfoById failed: lobby not found. lobby={}", Context->LobbyId
          );
          EOS_LobbySearch_Release(Context->SearchHandle);
          CompleteFetchCallback(Context->OnComplete, false, {});
          delete Context;
          return;
        }

        EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
        CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
        CopyOptions.LobbyIndex = 0;

        EOS_HLobbyDetails DetailsHandle = nullptr;
        EOS_EResult CopyResult = EOS_LobbySearch_CopySearchResultByIndex(
            Context->SearchHandle, &CopyOptions, &DetailsHandle
        );
        if (CopyResult != EOS_EResult::EOS_Success || !DetailsHandle) {
          M_LOG(
              "[EOSLobby] FetchLobbyInfoById CopySearchResult failed: {}", SafeEOSResult(CopyResult)
          );
          EOS_LobbySearch_Release(Context->SearchHandle);
          CompleteFetchCallback(Context->OnComplete, false, {});
          delete Context;
          return;
        }

        FLobbyInfo LobbyInfo = MakeLobbyInfoFromDetails(DetailsHandle);
        if (!LobbyInfo.bValid) {
          EOS_LobbyDetails_Release(DetailsHandle);
          EOS_LobbySearch_Release(Context->SearchHandle);
          CompleteFetchCallback(Context->OnComplete, false, {});
          delete Context;
          return;
        }

        LobbyManager.ClearCachedLobbyDetails();
        LobbyManager.ImplPtr->CachedLobbyDetails.push_back({LobbyInfo.LobbyId, DetailsHandle});
        EOS_LobbySearch_Release(Context->SearchHandle);
        CompleteFetchCallback(Context->OnComplete, true, LobbyInfo);
        delete Context;
      }
  );
}

void EOSLobbyManager::JoinLobby(const FLobbyInfo& LobbyInfo, std::function<void(bool)> OnComplete) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  EOS_HLobbyDetails DetailsHandle = FindCachedLobbyDetails(LobbyInfo.LobbyId);

  if (!LobbyInfo.bValid || LobbyInfo.LobbyId.empty() || !LobbyHandle || !LocalUserId ||
      !DetailsHandle) {
    M_LOG("[EOSLobby] JoinLobby failed: lobby info or cached details is invalid.");
    CompleteBoolCallback(OnComplete, false);
    return;
  }

  EOS_Lobby_JoinLobbyOptions Options = {};
  Options.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
  Options.LobbyDetailsHandle = DetailsHandle;
  Options.LocalUserId = LocalUserId;
  Options.bPresenceEnabled = EOS_FALSE;
  Options.LocalRTCOptions = nullptr;
  Options.bCrossplayOptOut = EOS_FALSE;
  Options.RTCRoomJoinActionType = EOS_ELobbyRTCRoomJoinActionType::EOS_LRRJAT_ManualJoin;

  auto* Context = new FJoinLobbyContext{LobbyInfo.LobbyId, std::move(OnComplete)};
  EOS_Lobby_JoinLobby(
      LobbyHandle, &Options, Context, [](const EOS_Lobby_JoinLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FJoinLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] JoinLobby failed: callback data is null.");
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success) {
          LobbyManager.ImplPtr->CurrentLobbyId =
              !Context->LobbyId.empty() ? Context->LobbyId : SafeText(Data->LobbyId);
          LobbyManager.ImplPtr->bInLobby = true;
          LobbyManager.RegisterLobbyNotifications();
          M_LOG("[EOSLobby] JoinLobby success");
          M_LOG("[EOSLobby] LobbyId = {}", LobbyManager.ImplPtr->CurrentLobbyId);
          CompleteBoolCallback(Context->OnComplete, true);
          delete Context;
          return;
        }

        M_LOG("[EOSLobby] JoinLobby failed: {}", SafeEOSResult(Data->ResultCode));
        CompleteBoolCallback(Context->OnComplete, false);
        delete Context;
      }
  );
}

bool EOSLobbyManager::IsInLobby() const { return ImplPtr->bInLobby; }

bool EOSLobbyManager::IsCurrentLobbyMember(const std::string& ProductUserId) const {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!ImplPtr->bInLobby || ImplPtr->CurrentLobbyId.empty() || ProductUserId.empty() ||
      !LobbyHandle || !LocalUserId) {
    return false;
  }

  EOS_Lobby_CopyLobbyDetailsHandleOptions CopyOptions = {};
  CopyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
  CopyOptions.LocalUserId = LocalUserId;
  CopyOptions.LobbyId = ImplPtr->CurrentLobbyId.c_str();

  EOS_HLobbyDetails DetailsHandle = nullptr;
  const EOS_EResult CopyResult =
      EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyOptions, &DetailsHandle);
  if (CopyResult != EOS_EResult::EOS_Success || !DetailsHandle) {
    M_LOG(
        "[EOSLobby] Lobby member authorization failed: copy details result={}",
        SafeEOSResult(CopyResult)
    );
    return false;
  }

  EOS_LobbyDetails_GetMemberCountOptions CountOptions = {};
  CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
  const uint32_t MemberCount = EOS_LobbyDetails_GetMemberCount(DetailsHandle, &CountOptions);
  bool bIsMember = false;
  for (uint32_t MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex) {
    EOS_LobbyDetails_GetMemberByIndexOptions MemberOptions = {};
    MemberOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
    MemberOptions.MemberIndex = MemberIndex;
    EOS_ProductUserId MemberUserId =
        EOS_LobbyDetails_GetMemberByIndex(DetailsHandle, &MemberOptions);
    if (ProductUserIdToString(MemberUserId) == ProductUserId) {
      bIsMember = true;
      break;
    }
  }

  EOS_LobbyDetails_Release(DetailsHandle);
  M_LOG(
      "[EOSLobby] Lobby member authorization: remoteUser={} member={} memberCount={}",
      ProductUserId,
      bIsMember,
      MemberCount
  );
  return bIsMember;
}

std::string EOSLobbyManager::GetCurrentLobbyId() const { return ImplPtr->CurrentLobbyId; }

void EOSLobbyManager::ForceLocalDisconnect(ELobbyDisconnectReason Reason) {
  HandleLocalDisconnect(Reason, true);
}

void EOSLobbyManager::SetOnLobbyDisconnected(std::function<void(ELobbyDisconnectReason)> Callback) {
  ImplPtr->OnLobbyDisconnected = std::move(Callback);
}

void EOSLobbyManager::ClearCachedLobbyDetails() {
  for (FCachedLobbyDetails& CachedLobbyDetail : ImplPtr->CachedLobbyDetails) {
    if (CachedLobbyDetail.DetailsHandle) {
      EOS_LobbyDetails_Release(CachedLobbyDetail.DetailsHandle);
      CachedLobbyDetail.DetailsHandle = nullptr;
    }
  }

  ImplPtr->CachedLobbyDetails.clear();
}

EOS_HLobbyDetails EOSLobbyManager::FindCachedLobbyDetails(const std::string& LobbyId) const {
  auto It = std::find_if(
      ImplPtr->CachedLobbyDetails.begin(),
      ImplPtr->CachedLobbyDetails.end(),
      [&LobbyId](const FCachedLobbyDetails& CachedLobbyDetail) {
        return CachedLobbyDetail.LobbyId == LobbyId;
      }
  );

  return It != ImplPtr->CachedLobbyDetails.end() ? It->DetailsHandle : nullptr;
}

void EOSLobbyManager::RegisterLobbyNotifications() {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  if (!LobbyHandle) {
    M_LOG("[EOSLobby] Register notifications skipped: lobby handle is null.");
    return;
  }

  UnregisterLobbyNotifications();

  EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions MemberStatusOptions = {};
  MemberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
  ImplPtr->MemberStatusNotificationId = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(
      LobbyHandle, &MemberStatusOptions, this, &EOSLobbyManager::OnLobbyMemberStatusReceived
  );
  if (ImplPtr->MemberStatusNotificationId == EOS_INVALID_NOTIFICATIONID) {
    M_LOG("[EOSLobby] AddNotifyLobbyMemberStatusReceived failed.");
  }

  EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions LobbyUpdateOptions = {};
  LobbyUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;
  ImplPtr->LobbyUpdateNotificationId = EOS_Lobby_AddNotifyLobbyUpdateReceived(
      LobbyHandle, &LobbyUpdateOptions, this, &EOSLobbyManager::OnLobbyUpdateReceived
  );
  if (ImplPtr->LobbyUpdateNotificationId == EOS_INVALID_NOTIFICATIONID) {
    M_LOG("[EOSLobby] AddNotifyLobbyUpdateReceived failed.");
  }
}

void EOSLobbyManager::UnregisterLobbyNotifications() {
  const EOS_NotificationId MemberStatusId = ImplPtr->MemberStatusNotificationId;
  const EOS_NotificationId LobbyUpdateId = ImplPtr->LobbyUpdateNotificationId;
  ImplPtr->MemberStatusNotificationId = EOS_INVALID_NOTIFICATIONID;
  ImplPtr->LobbyUpdateNotificationId = EOS_INVALID_NOTIFICATIONID;

  if (MemberStatusId == EOS_INVALID_NOTIFICATIONID && LobbyUpdateId == EOS_INVALID_NOTIFICATIONID) {
    return;
  }

  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  if (!LobbyHandle) {
    return;
  }

  if (MemberStatusId != EOS_INVALID_NOTIFICATIONID) {
    EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, MemberStatusId);
  }

  if (LobbyUpdateId != EOS_INVALID_NOTIFICATIONID) {
    EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, LobbyUpdateId);
  }
}

void EOSLobbyManager::HandleLocalDisconnect(ELobbyDisconnectReason Reason, bool bNotify) {
  if (!ImplPtr->bInLobby) {
    return;
  }

  M_LOG("[EOSLobby] Local lobby disconnected: reason={}", LobbyDisconnectReasonToString(Reason));
  ImplPtr->bInLobby = false;
  ImplPtr->CurrentLobbyId.clear();
  UnregisterLobbyNotifications();
  ClearCachedLobbyDetails();

  if (bNotify && ImplPtr->OnLobbyDisconnected) {
    ImplPtr->OnLobbyDisconnected(Reason);
  }
}

void EOSLobbyManager::HandleMemberStatusReceived(
    const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data
) {
  if (!Data || !ImplPtr->bInLobby) {
    return;
  }

  M_LOG(
      "[EOSLobby] Member status received: lobby={}, status={}",
      SafeText(Data->LobbyId),
      LobbyMemberStatusToString(Data->CurrentStatus)
  );

  if (ImplPtr->CurrentLobbyId != SafeText(Data->LobbyId)) {
    return;
  }

  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!IsSameProductUserId(LocalUserId, Data->TargetUserId)) {
    return;
  }

  ELobbyDisconnectReason Reason = ELobbyDisconnectReason::NetworkError;
  if (TryGetDisconnectReasonFromMemberStatus(Data->CurrentStatus, Reason)) {
    HandleLocalDisconnect(Reason, true);
    return;
  }

  if (Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED) {
    M_LOG("[EOSLobby] Local user was promoted to lobby owner.");
  }
}

void EOSLobbyManager::HandleLobbyUpdateReceived(
    const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data
) {
  if (!Data || !ImplPtr->bInLobby || ImplPtr->CurrentLobbyId != SafeText(Data->LobbyId)) {
    return;
  }

  if (IsCurrentLobbyUpdateClosed(Data)) {
    HandleLocalDisconnect(ELobbyDisconnectReason::LobbyClosed, true);
  }
}

bool EOSLobbyManager::IsCurrentLobbyUpdateClosed(
    const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data
) const {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!Data || !LobbyHandle || !LocalUserId) {
    return true;
  }

  EOS_Lobby_CopyLobbyDetailsHandleOptions Options = {};
  Options.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
  Options.LobbyId = Data->LobbyId;
  Options.LocalUserId = LocalUserId;

  EOS_HLobbyDetails DetailsHandle = nullptr;
  EOS_EResult Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &Options, &DetailsHandle);
  if (Result == EOS_EResult::EOS_Success && DetailsHandle) {
    EOS_LobbyDetails_Release(DetailsHandle);
    M_LOG("[EOSLobby] Lobby update received: lobby={}", SafeText(Data->LobbyId));
    return false;
  }

  M_LOG(
      "[EOSLobby] Lobby update indicates closed lobby: lobby={}, result={}",
      SafeText(Data->LobbyId),
      SafeEOSResult(Result)
  );
  return true;
}

void EOS_CALL EOSLobbyManager::OnLobbyMemberStatusReceived(
    const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data
) {
  auto* LobbyManager = static_cast<EOSLobbyManager*>(Data ? Data->ClientData : nullptr);
  if (LobbyManager) {
    LobbyManager->HandleMemberStatusReceived(Data);
  }
}

void EOS_CALL
EOSLobbyManager::OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data) {
  auto* LobbyManager = static_cast<EOSLobbyManager*>(Data ? Data->ClientData : nullptr);
  if (LobbyManager) {
    LobbyManager->HandleLobbyUpdateReceived(Data);
  }
}
