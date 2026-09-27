#pragma once

#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FPath {
 public:
  FPath() = default;
  explicit FPath(std::string Value);

  const std::string& String() const { return Value; }
  bool Empty() const { return Value.empty(); }

  friend bool operator==(const FPath&, const FPath&) = default;

 private:
  std::string Value;
};
