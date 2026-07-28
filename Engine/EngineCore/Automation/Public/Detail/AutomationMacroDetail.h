#pragma once

#include "Detail/AutomationMethodRegistrar.h"

#define BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right) Left##Right
#define BROCCOLI_AUTOMATION_CONCAT(Left, Right) BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right)

#define BROCCOLI_AUTOMATION_SELECT_METHOD(_1, _2, _3, _4, _5, _6, Selected, ...) Selected
#define BROCCOLI_AUTOMATION_METHOD_DISPATCH(...) \
  BROCCOLI_AUTOMATION_METHOD_DISPATCH_IMPL(      \
      __VA_ARGS__,                               \
      BROCCOLI_REGISTER_AUTOMATION_METHOD_6,     \
      BROCCOLI_REGISTER_AUTOMATION_METHOD_5,     \
      BROCCOLI_REGISTER_AUTOMATION_METHOD_4,     \
      BROCCOLI_AUTOMATION_DISPATCH_SENTINEL      \
  )

#define BROCCOLI_AUTOMATION_METHOD_DISPATCH_IMPL(...) BROCCOLI_AUTOMATION_SELECT_METHOD(__VA_ARGS__)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_4(Name, Description, Permission, Method) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(__COUNTER__, Name, Description, Permission, Method)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_5(Name, Description, Permission, Method, Parameters) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(                                                      \
      __COUNTER__, Name, Description, Permission, Method, Parameters                             \
  )

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_6(                                      \
    Name, Description, Permission, Method, Parameters, ResultAdapter                \
)                                                                                   \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(                                         \
      __COUNTER__, Name, Description, Permission, Method, Parameters, ResultAdapter \
  )

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(Counter, ...) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, __VA_ARGS__)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, ...)                      \
  namespace {                                                                                \
  void BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter)(                 \
      FAutomationMethodRegistry & ActorRegistry,                                             \
      FAutomationComponentMethodRegistry& ComponentRegistry                                  \
  ) {                                                                                        \
    BroccoliAutomationDetail::RegisterMethod(ActorRegistry, ComponentRegistry, __VA_ARGS__); \
  }                                                                                          \
  const FAutomationUnifiedMethodAutoRegister                                                 \
      BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationAutoRegister_, Counter)(                  \
          &BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter)              \
      );                                                                                     \
  }
