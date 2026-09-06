#include "LogController.h"
#include "../Detail/HttpControllerUtilities.h"

FAutomationLogController::FAutomationLogController(
    FAutomationHttpRequestExecutor& InExecutor, FAutomationCommandQueue& InCommandQueue
)
    : FAutomationHttpControllerBase(InExecutor), CommandQueue(InCommandQueue) {}

FAutomationHttpResponse FAutomationLogController::GetRecentLogs(
    const FAutomationLogQueryText& Query
) {
  if (Query.bHasUnknownParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidRequest, "The log query contains an unknown parameter."
        )
    };
  }
  if (Query.bHasDuplicateParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "Each log query parameter may be specified only once."
        )
    };
  }

  FLogQuery LogQuery;
  if (Query.Limit) {
    uint64_t Limit = 0;
    if (!TryParseUnsigned(*Query.Limit, Limit) || Limit < 1 || Limit > 1000) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument,
              "limit must be an unsigned integer between 1 and 1000."
          )
      };
    }
    LogQuery.Limit = static_cast<size_t>(Limit);
  }
  if (Query.Level) {
    LogQuery.MinimumLevel = ParseLogLevel(*Query.Level);
    if (!LogQuery.MinimumLevel) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument,
              "level must be debug, log, info, warning, or error."
          )
      };
    }
  }
  if (Query.AfterSequence) {
    uint64_t AfterSequence = 0;
    if (!TryParseUnsigned(*Query.AfterSequence, AfterSequence)) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument,
              "afterSequence must be an unsigned 64-bit integer."
          )
      };
    }
    LogQuery.AfterSequence = AfterSequence;
  }

  try {
    const FLogQueryResult Result = MLog::GetRecentEntries(LogQuery);
    nlohmann::json Entries = nlohmann::json::array();
    for (const FLogEntry& Entry : Result.Entries) {
      Entries.push_back(
          {{"sequence", Entry.Sequence},
           {"timestamp", MLog::FormatTimestamp(Entry.Timestamp)},
           {"level", std::string(MLog::ToLevelString(Entry.Level))},
           {"category", Entry.Category},
           {"message", Entry.Message}}
      );
    }
    return {
        200,
        MakeAutomationSuccess(
            {{"entries", std::move(Entries)},
             {"count", Result.Entries.size()},
             {"oldestAvailableSequence", Result.OldestAvailableSequence},
             {"latestSequence", Result.LatestSequence},
             {"nextAfterSequence", Result.NextAfterSequence},
             {"droppedEntries", Result.DroppedEntries},
             {"historyLost", Result.bHistoryLost},
             {"hasMore", Result.bHasMore}}
        )
    };
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

