#pragma once

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

struct FAutomationParameterMetadata {
  std::string Name;
  std::string Description;
};

class FAutomationParameterMetadataList {
 public:
  FAutomationParameterMetadataList() = default;
  FAutomationParameterMetadataList(std::initializer_list<FAutomationParameterMetadata> InParameters)
      : Parameters(InParameters) {}

  const std::vector<FAutomationParameterMetadata>& GetParameters() const { return Parameters; }

 private:
  std::vector<FAutomationParameterMetadata> Parameters;
};

namespace BroccoliAutomationDetail {

inline FAutomationParameterMetadata MakeParameterMetadata(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description)};
}

}  // namespace BroccoliAutomationDetail

#define BROCCOLI_AUTOMATION_PARAM_IMPL(Name, Description) \
  BroccoliAutomationDetail::MakeParameterMetadata(Name, Description)

#define BROCCOLI_AUTOMATION_PARAMS_IMPL(...) (FAutomationParameterMetadataList{__VA_ARGS__})
