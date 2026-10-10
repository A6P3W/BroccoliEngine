#pragma once

#include "Detail/AutomationMethodBinding.h"

#define BROCCOLI_CONTROL_CONCAT_IMPL(Left, Right) Left##Right
#define BROCCOLI_CONTROL_CONCAT(Left, Right) BROCCOLI_CONTROL_CONCAT_IMPL(Left, Right)

#define BROCCOLI_REGISTER_CONTROL_CLASS_IMPL(T, Counter)                  \
  namespace {                                                             \
  void BROCCOLI_CONTROL_CONCAT(BroccoliControlRegistration_, Counter)(    \
      BroccoliAutomationDetail::FAutomationRegistrationContext & Context  \
  ) {                                                                     \
    BroccoliAutomationDetail::RegisterClass<T>(Context);                  \
  }                                                                       \
  const BroccoliAutomationDetail::FAutomationRegistrationToken            \
      BROCCOLI_CONTROL_CONCAT(BroccoliControlAutoRegister_, Counter)(     \
          &BROCCOLI_CONTROL_CONCAT(BroccoliControlRegistration_, Counter) \
      );                                                                  \
  }

#define REGISTER_CONTROL_CLASS(T) BROCCOLI_REGISTER_CONTROL_CLASS_IMPL(T, __COUNTER__)
