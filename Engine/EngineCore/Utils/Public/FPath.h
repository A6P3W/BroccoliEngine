#pragma once

#include <cstddef>
#include <string>
#include <string_view>

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

// Returns true when Value is valid UTF-8 with at most MaxLength Unicode scalar values.
BROCCOLI_ENGINE_API bool IsValidUnicodeScalarString(std::string_view Value, std::size_t MaxLength);
