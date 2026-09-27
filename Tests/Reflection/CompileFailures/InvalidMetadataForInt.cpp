#include "ReflectionGenerator.h"

struct FInvalidMetadataForInt {
  EDITOR_PROPERTY(.MaxLength = 32)
  int Value = 0;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FInvalidMetadataForInt>("InvalidInt");
