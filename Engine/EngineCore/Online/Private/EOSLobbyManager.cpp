#include "EOSLobbyManager.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <eos_lobby.h>

#include "EOSAuthManager.h"
#include "EOSCoreManager.h"
#include "Log.h"

namespace {
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

EOS_ProductUserId GetLoggedInUserId() {
  EOSAuthManager& AuthManager = EOSAuthManager::Get();
  return AuthManager.IsLoggedIn() ? AuthManager.GetLocalUserId() : nullptr;
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
  LobbyInfo.MaxMembers = ClampToInt(DetailsInfo->MaxMembers);
  LobbyInfo.CurrentMembers = ClampToInt(DetailsInfo->MaxMembers - (std::min)(DetailsInfo->AvailableSlots, DetailsInfo->MaxMembers));
  LobbyInfo.bValid = !LobbyInfo.LobbyId.empty();

  EOS_LobbyDetails_Info_Release(DetailsInfo);
  return LobbyInfo;
}

struct FCreateLobbyContext {
  FCreateLobbyRequest Request;
  std::function<void(bool, const FLobbyInfo&)> OnComplete;
};

struct FLeaveLobbyContext {
  std::string LobbyId;
  std::function<void(bool)> OnComplete;
};

struct FSearchLobbiesContext {
  EOS_HLobbySearch SearchHandle = nullptr;
  std::function<void(bool, const std::vector<FLobbyInfo>&)> OnComplete;
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
}  // namespace

EOSLobbyManager& EOSLobbyManager::Get() {
  static EOSLobbyManager Instance;
  return Instance;
}

EOSLobbyManager::~EOSLobbyManager() { ClearCachedLobbyDetails(); }

void EOSLobbyManager::CreateLobby(
    const FCreateLobbyRequest& Request,
    std::function<void(bool, const FLobbyInfo&)> OnComplete
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
  Options.PermissionLevel = Request.bPublicAdvertised ? EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED
                                                      : EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY;
  Options.bPresenceEnabled = EOS_FALSE;
  Options.bAllowInvites = EOS_TRUE;
  Options.BucketId = GetSafeBucketId(Request);
  Options.bDisableHostMigration = EOS_FALSE;
  Options.bEnableRTCRoom = EOS_FALSE;
  Options.LocalRTCOptions = nullptr;
  Options.LobbyId = nullptr;
  Options.bEnableJoinById = EOS_FALSE;
  Options.bRejoinAfterKickRequiresInvite = EOS_FALSE;
  Options.AllowedPlatformIds = nullptr;
  Options.AllowedPlatformIdsCount = 0;
  Options.bCrossplayOptOut = EOS_FALSE;
  Options.RTCRoomJoinActionType = EOS_ELobbyRTCRoomJoinActionType::EOS_LRRJAT_ManualJoin;

  auto* Context = new FCreateLobbyContext{Request, std::move(OnComplete)};
  EOS_Lobby_CreateLobby(
      LobbyHandle,
      &Options,
      Context,
      [](const EOS_Lobby_CreateLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FCreateLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] CreateLobby failed: callback data is null.");
          LobbyManager.CurrentLobbyId.clear();
          LobbyManager.bInLobby = false;
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success && Data->LobbyId) {
          FLobbyInfo LobbyInfo;
          LobbyInfo.LobbyId = Data->LobbyId;
          LobbyInfo.CurrentMembers = 1;
          LobbyInfo.MaxMembers = (std::max)(1, Context->Request.MaxMembers);
          LobbyInfo.bValid = true;

          LobbyManager.CurrentLobbyId = LobbyInfo.LobbyId;
          LobbyManager.bInLobby = true;

          M_LOG("[EOSLobby] CreateLobby success");
          M_LOG("[EOSLobby] LobbyId = {}", LobbyManager.CurrentLobbyId);
          CompleteCreateCallback(Context->OnComplete, true, LobbyInfo);
          delete Context;
          return;
        }

        M_LOG("[EOSLobby] CreateLobby failed: {}", SafeEOSResult(Data->ResultCode));
        LobbyManager.CurrentLobbyId.clear();
        LobbyManager.bInLobby = false;
        CompleteCreateCallback(Context->OnComplete, false, {});
        delete Context;
      }
  );
}

void EOSLobbyManager::LeaveLobby(std::function<void(bool)> OnComplete) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  if (!LobbyHandle || !LocalUserId || !bInLobby || CurrentLobbyId.empty()) {
    M_LOG("[EOSLobby] LeaveLobby failed: current lobby state is invalid.");
    CompleteBoolCallback(OnComplete, false);
    return;
  }

  auto* Context = new FLeaveLobbyContext{CurrentLobbyId, std::move(OnComplete)};

  EOS_Lobby_LeaveLobbyOptions Options = {};
  Options.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
  Options.LocalUserId = LocalUserId;
  Options.LobbyId = Context->LobbyId.c_str();
  EOS_Lobby_LeaveLobby(
      LobbyHandle,
      &Options,
      Context,
      [](const EOS_Lobby_LeaveLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FLeaveLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] LeaveLobby failed: callback data is null.");
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success ||
            Data->ResultCode == EOS_EResult::EOS_NotFound) {
          LobbyManager.CurrentLobbyId.clear();
          LobbyManager.bInLobby = false;
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
  EOS_EResult CreateSearchResult = EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &SearchHandle);
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

  EOS_LobbySearch_FindOptions FindOptions = {};
  FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
  FindOptions.LocalUserId = LocalUserId;

  auto* Context = new FSearchLobbiesContext{SearchHandle, std::move(OnComplete)};
  EOS_LobbySearch_Find(
      SearchHandle,
      &FindOptions,
      Context,
      [](const EOS_LobbySearch_FindCallbackInfo* Data) {
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
        uint32_t ResultCount = EOS_LobbySearch_GetSearchResultCount(Context->SearchHandle, &CountOptions);

        Results.reserve(ResultCount);
        LobbyManager.CachedLobbyDetails.reserve(ResultCount);

        for (uint32_t Index = 0; Index < ResultCount; ++Index) {
          EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
          CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
          CopyOptions.LobbyIndex = Index;

          EOS_HLobbyDetails DetailsHandle = nullptr;
          EOS_EResult CopyResult =
              EOS_LobbySearch_CopySearchResultByIndex(Context->SearchHandle, &CopyOptions, &DetailsHandle);
          if (CopyResult != EOS_EResult::EOS_Success || !DetailsHandle) {
            M_LOG("[EOSLobby] CopySearchResultByIndex failed: index={}, result={}", Index, SafeEOSResult(CopyResult));
            continue;
          }

          FLobbyInfo LobbyInfo = MakeLobbyInfoFromDetails(DetailsHandle);
          if (!LobbyInfo.bValid) {
            EOS_LobbyDetails_Release(DetailsHandle);
            continue;
          }

          Results.push_back(LobbyInfo);
          LobbyManager.CachedLobbyDetails.push_back({LobbyInfo.LobbyId, DetailsHandle});
        }

        EOS_LobbySearch_Release(Context->SearchHandle);
        M_LOG("[EOSLobby] SearchLobbies success");
        M_LOG("[EOSLobby] Found lobby count = {}", Results.size());
        CompleteSearchCallback(Context->OnComplete, true, Results);
        delete Context;
      }
  );
}

void EOSLobbyManager::JoinLobby(const FLobbyInfo& LobbyInfo, std::function<void(bool)> OnComplete) {
  EOS_HLobby LobbyHandle = EOSCoreManager::Get().GetLobbyHandle();
  EOS_ProductUserId LocalUserId = GetLoggedInUserId();
  EOS_HLobbyDetails DetailsHandle = FindCachedLobbyDetails(LobbyInfo.LobbyId);

  if (!LobbyInfo.bValid || LobbyInfo.LobbyId.empty() || !LobbyHandle || !LocalUserId || !DetailsHandle) {
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
      LobbyHandle,
      &Options,
      Context,
      [](const EOS_Lobby_JoinLobbyCallbackInfo* Data) {
        auto* Context = static_cast<FJoinLobbyContext*>(Data ? Data->ClientData : nullptr);
        EOSLobbyManager& LobbyManager = EOSLobbyManager::Get();

        if (!Data || !Context) {
          M_LOG("[EOSLobby] JoinLobby failed: callback data is null.");
          return;
        }

        if (Data->ResultCode == EOS_EResult::EOS_Success) {
          LobbyManager.CurrentLobbyId = !Context->LobbyId.empty() ? Context->LobbyId : SafeText(Data->LobbyId);
          LobbyManager.bInLobby = true;
          M_LOG("[EOSLobby] JoinLobby success");
          M_LOG("[EOSLobby] LobbyId = {}", LobbyManager.CurrentLobbyId);
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

bool EOSLobbyManager::IsInLobby() const { return bInLobby; }

std::string EOSLobbyManager::GetCurrentLobbyId() const { return CurrentLobbyId; }

void EOSLobbyManager::ClearCachedLobbyDetails() {
  for (FCachedLobbyDetails& CachedLobbyDetail : CachedLobbyDetails) {
    if (CachedLobbyDetail.DetailsHandle) {
      EOS_LobbyDetails_Release(CachedLobbyDetail.DetailsHandle);
      CachedLobbyDetail.DetailsHandle = nullptr;
    }
  }

  CachedLobbyDetails.clear();
}

EOS_HLobbyDetails EOSLobbyManager::FindCachedLobbyDetails(const std::string& LobbyId) const {
  auto It = std::find_if(
      CachedLobbyDetails.begin(),
      CachedLobbyDetails.end(),
      [&LobbyId](const FCachedLobbyDetails& CachedLobbyDetail) {
        return CachedLobbyDetail.LobbyId == LobbyId;
      }
  );

  return It != CachedLobbyDetails.end() ? It->DetailsHandle : nullptr;
}
