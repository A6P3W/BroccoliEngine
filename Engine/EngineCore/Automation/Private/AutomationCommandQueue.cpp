#include "AutomationCommandQueue.h"

#include <exception>
#include <utility>

namespace {
constexpr std::string_view EngineShuttingDownMessage = "The engine is shutting down.";
constexpr std::string_view RequestCancelledMessage = "The request was cancelled before execution.";
constexpr std::string_view UnknownExceptionMessage =
    "The automation command failed with an unknown exception.";
}  // namespace

bool FAutomationCommandTicket::IsValid() const { return Command != nullptr && Result.valid(); }

void FAutomationCommandTicket::Cancel() {
  if (!Command) {
    return;
  }

  std::scoped_lock Lock(Command->StateMutex);
  if (Command->bStarted.load()) {
    return;
  }
  Command->bCancelled.store(true);
}

FAutomationCommandQueue::~FAutomationCommandQueue() {
  StopAcceptingCommands();
  CancelAll(EAutomationErrorCode::EngineShuttingDown, EngineShuttingDownMessage);
}

FAutomationCommandTicket FAutomationCommandQueue::Enqueue(FAutomationTask Task) {
  if (!Task) {
    return {};
  }

  const uint64_t RequestId = NextRequestId.fetch_add(1);
  FAutomationCommandPtr Command = std::make_shared<FAutomationCommand>();
  Command->RequestId = RequestId;
  Command->Execute = std::move(Task);

  FAutomationCommandTicket Ticket;
  Ticket.RequestId = RequestId;
  Ticket.Command = Command;
  Ticket.Result = Command->ResultPromise.get_future();

  {
    std::scoped_lock Lock(QueueMutex);
    if (bAcceptingCommands.load()) {
      PendingCommands.push_back(Command);
      return Ticket;
    }
  }

  CompleteCommand(
      Command,
      MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, EngineShuttingDownMessage)
  );
  return Ticket;
}

void FAutomationCommandQueue::ProcessCommands() {
  std::deque<FAutomationCommandPtr> Commands;

  {
    std::scoped_lock Lock(QueueMutex);
    Commands.swap(PendingCommands);
  }

  for (const FAutomationCommandPtr& Command : Commands) {
    bool Cancelled = false;
    {
      std::scoped_lock Lock(Command->StateMutex);
      Cancelled = Command->bCancelled.load();
      if (!Cancelled) {
        Command->bStarted.store(true);
      }
    }

    if (Cancelled) {
      CompleteCommand(
          Command,
          MakeAutomationError(EAutomationErrorCode::RequestTimeout, RequestCancelledMessage)
      );
      continue;
    }

    try {
      CompleteCommand(Command, Command->Execute());
    } catch (const std::exception&) {
      CompleteCommand(
          Command, MakeAutomationError(EAutomationErrorCode::InternalError, UnknownExceptionMessage)
      );
    } catch (...) {
      CompleteCommand(
          Command, MakeAutomationError(EAutomationErrorCode::InternalError, UnknownExceptionMessage)
      );
    }
  }
}

void FAutomationCommandQueue::StopAcceptingCommands() {
  std::scoped_lock Lock(QueueMutex);
  bAcceptingCommands.store(false);
}

void FAutomationCommandQueue::CancelAll(EAutomationErrorCode ErrorCode, std::string_view Message) {
  std::deque<FAutomationCommandPtr> Commands;

  {
    std::scoped_lock Lock(QueueMutex);
    Commands.swap(PendingCommands);
  }

  for (const FAutomationCommandPtr& Command : Commands) {
    {
      std::scoped_lock Lock(Command->StateMutex);
      Command->bCancelled.store(true);
    }
    CompleteCommand(Command, MakeAutomationError(ErrorCode, Message));
  }
}

size_t FAutomationCommandQueue::GetPendingCommandCount() const {
  std::scoped_lock Lock(QueueMutex);
  return PendingCommands.size();
}

bool FAutomationCommandQueue::IsAcceptingCommands() const { return bAcceptingCommands.load(); }

void FAutomationCommandQueue::CompleteCommand(
    const FAutomationCommandPtr& Command, nlohmann::json Result
) {
  if (!Command || Command->bCompleted.exchange(true)) {
    return;
  }

  Command->ResultPromise.set_value(std::move(Result));
}
