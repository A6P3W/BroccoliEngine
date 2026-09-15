#pragma once

#include "HttpControllerBase.h"

#include "Runtime/RuntimeState.h"

class FAutomationLogController final : public FAutomationHttpControllerBase {
 public:
  FAutomationLogController(
      FAutomationHttpRequestExecutor& InExecutor, FAutomationCommandQueue& InCommandQueue
  );

  FAutomationHttpResponse GetRecentLogs(const FAutomationLogQueryText& Query);

 private:
  FAutomationCommandQueue& CommandQueue;
};
