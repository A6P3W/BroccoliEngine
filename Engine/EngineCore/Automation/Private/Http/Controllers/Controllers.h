#pragma once

#include "DiscoveryController.h"
#include "InvocationController.h"
#include "LogController.h"
#include "SystemController.h"
#include "WorldController.h"

struct FAutomationHttpControllers {
  FAutomationWorldController& World;
  FAutomationDiscoveryController& Discovery;
  FAutomationInvocationController& Invocation;
  FAutomationSystemController& System;
  FAutomationLogController& Log;
};
