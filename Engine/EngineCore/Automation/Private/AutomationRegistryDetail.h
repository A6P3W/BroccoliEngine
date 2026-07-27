#pragma once

#include <string>
#include <string_view>

#include "Log.h"

namespace AutomationRegistryDetail {
inline bool RejectRegistration(
    std::string Message, std::string* OutError, std::string_view RegistryName
) {
  if (OutError) {
    *OutError = Message;
  }

  M_LOG("Automation {} registration rejected: {}", RegistryName, Message);
  return false;
}
}  // namespace AutomationRegistryDetail
