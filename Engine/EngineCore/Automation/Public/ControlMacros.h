#pragma once

#include "Detail/AutomationMethodBinding.h"

#define BROCCOLI_CONTROL_CONCAT_IMPL(Left, Right) Left##Right
#define BROCCOLI_CONTROL_CONCAT(Left, Right) BROCCOLI_CONTROL_CONCAT_IMPL(Left, Right)

#define BROCCOLI_CONTROL_METHOD_IMPL(Counter, ...) \
  BROCCOLI_CONTROL_METHOD_IMPL_EXPANDED(Counter, __VA_ARGS__)

#define BROCCOLI_CONTROL_METHOD_IMPL_EXPANDED(Counter, ...)               \
  namespace {                                                             \
  void BROCCOLI_CONTROL_CONCAT(BroccoliControlRegistration_, Counter)(    \
      BroccoliAutomationDetail::FAutomationRegistrationContext & Context  \
  ) {                                                                     \
    BroccoliAutomationDetail::RegisterMethod(Context, __VA_ARGS__);       \
  }                                                                       \
  const BroccoliAutomationDetail::FAutomationRegistrationToken            \
      BROCCOLI_CONTROL_CONCAT(BroccoliControlAutoRegister_, Counter)(     \
          &BROCCOLI_CONTROL_CONCAT(BroccoliControlRegistration_, Counter) \
      );                                                                  \
  }

#define CONTROL_METHOD(...) BROCCOLI_CONTROL_METHOD_IMPL(__COUNTER__, __VA_ARGS__)

#define CONTROL_PARAMETERS(...) (BroccoliAutomationDetail::MakeParameterMetadataList(__VA_ARGS__))
#define CONTROL_PARAMETER(Name, Description) \
  BroccoliAutomationDetail::MakeParameterMetadata(Name, Description)
