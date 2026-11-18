#include "FPath.h"

#include <limits>
#include <stdexcept>
#include <utility>

#include "PathResolver.h"

FPath::FPath(std::string Input) {
  auto Normalized = PathResolver::MakeVirtualPath(Input);
  if (!Normalized) throw std::invalid_argument("Path is outside the resource roots");
  if (!IsValidUnicodeScalarString(*Normalized, std::numeric_limits<std::size_t>::max()))
    throw std::invalid_argument("Path is not valid UTF-8");
  Value = std::move(*Normalized);
}

bool IsValidUnicodeScalarString(std::string_view Value, std::size_t MaxLength) {
  std::size_t Count = 0;
  for (std::size_t Index = 0; Index < Value.size();) {
    const unsigned char First = static_cast<unsigned char>(Value[Index]);
    int Length = 0;
    unsigned int CodePoint = 0;
    if (First <= 0x7F) {
      Length = 1;
      CodePoint = First;
    } else if (First >= 0xC2 && First <= 0xDF) {
      Length = 2;
      CodePoint = First & 0x1F;
    } else if (First >= 0xE0 && First <= 0xEF) {
      Length = 3;
      CodePoint = First & 0x0F;
    } else if (First >= 0xF0 && First <= 0xF4) {
      Length = 4;
      CodePoint = First & 0x07;
    } else {
      return false;
    }
    if (Index + Length > Value.size()) return false;
    for (int Offset = 1; Offset < Length; ++Offset) {
      const unsigned char Next = static_cast<unsigned char>(Value[Index + Offset]);
      if ((Next & 0xC0) != 0x80) return false;
      CodePoint = (CodePoint << 6) | (Next & 0x3F);
    }
    if ((Length == 2 && CodePoint < 0x80) || (Length == 3 && CodePoint < 0x800) ||
        (Length == 4 && CodePoint < 0x10000) || (CodePoint >= 0xD800 && CodePoint <= 0xDFFF) ||
        CodePoint > 0x10FFFF)
      return false;
    ++Count;
    if (Count > MaxLength) return false;
    Index += Length;
  }
  return true;
}
