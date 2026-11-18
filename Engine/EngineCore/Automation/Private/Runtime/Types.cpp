#include <string>
#include <utility>

#include "AutomationTypes.h"

bool IsValidAutomationOperationName(std::string_view Name) {
  if (Name.empty() || Name.size() > 128 || Name.front() < 'a' || Name.front() > 'z') {
    return false;
  }
  for (const char Character : Name.substr(1)) {
    if ((Character < 'a' || Character > 'z') && (Character < '0' || Character > '9') &&
        Character != '_') {
      return false;
    }
  }
  return true;
}

nlohmann::json MakeAutomationSuccess(nlohmann::json Data) {
  return {{"success", true}, {"data", std::move(Data)}};
}
