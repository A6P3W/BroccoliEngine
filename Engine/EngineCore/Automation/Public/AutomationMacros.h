#pragma once

#include "AutomationTypes.h"
#include "Detail/AutomationMacroDetail.h"
#include "Detail/AutomationParameterMetadata.h"

#define REGISTER_AUTOMATION_METHOD(...) \
  BROCCOLI_AUTOMATION_METHOD_DISPATCH(__VA_ARGS__)(__VA_ARGS__)

#define AUTOMATION_PARAMS(...) BROCCOLI_AUTOMATION_PARAMS_IMPL(__VA_ARGS__)
#define AUTOMATION_PARAM(Name, Description) BROCCOLI_AUTOMATION_PARAM_IMPL(Name, Description)
