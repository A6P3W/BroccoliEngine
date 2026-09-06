#pragma once

#include "HttpControllerBase.h"

class FAutomationLogController final : public FAutomationHttpControllerBase {
 public:
  FAutomationLogController(
      FAutomationHttpRequestExecutor& InExecutor, FAutomationCommandQueue& InCommandQueue
  );

  FAutomationHttpResponse GetRecentLogs(const FAutomationLogQueryText& Query);

 private:
  FAutomationCommandQueue& CommandQueue;
};

