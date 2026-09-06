#pragma once

#include "Detail/AutomationMacroImplementation.h"

#define BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right) Left##Right
#define BROCCOLI_AUTOMATION_CONCAT(Left, Right) BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(Counter, ...) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, __VA_ARGS__)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, ...)         \
  namespace {                                                                   \
  void BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter)(    \
      BroccoliAutomationDetail::FAutomationRegistrationContext & Context        \
  ) {                                                                           \
    BroccoliAutomationDetail::RegisterMethod(Context, __VA_ARGS__);             \
  }                                                                             \
  const BroccoliAutomationDetail::FAutomationRegistrationToken                  \
      BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationAutoRegister_, Counter)(     \
          &BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter) \
      );                                                                        \
  }

#define REGISTER_AUTOMATION_METHOD(...) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(__COUNTER__, __VA_ARGS__)

#define AUTOMATION_PARAMS(...) (BroccoliAutomationDetail::MakeParameterMetadataList(__VA_ARGS__))
#define AUTOMATION_PARAM(Name, Description) \
  BroccoliAutomationDetail::MakeParameterMetadata(Name, Description)
