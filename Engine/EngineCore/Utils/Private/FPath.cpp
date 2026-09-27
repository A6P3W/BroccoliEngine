#include "FPath.h"

#include <limits>
#include <stdexcept>
#include <utility>

#include "PathResolver.h"
#include "Reflection.h"

FPath::FPath(std::string Input) {
  auto Normalized = PathResolver::MakeVirtualPath(Input);
  if (!Normalized) throw std::invalid_argument("Path is outside the resource roots");
  if (!IsValidUnicodeScalarString(*Normalized, std::numeric_limits<std::size_t>::max()))
    throw std::invalid_argument("Path is not valid UTF-8");
  Value = std::move(*Normalized);
}
