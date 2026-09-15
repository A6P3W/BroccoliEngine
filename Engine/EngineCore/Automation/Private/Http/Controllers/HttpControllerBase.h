#pragma once

#include "Http/HttpExecutor.h"

class FAutomationHttpControllerBase {
 protected:
  explicit FAutomationHttpControllerBase(FAutomationHttpRequestExecutor& InExecutor)
      : Executor(InExecutor) {}

  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket) {
    return Executor.WaitForResult(std::move(Ticket));
  }

  FAutomationHttpRequestExecutor& Executor;
};
