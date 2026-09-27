#include "ReflectionGenerator.h"

struct FInvalidMetadataForBool {
  EDITOR_PROPERTY(.Min = 0)
  bool Value = false;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FInvalidMetadataForBool>("InvalidBool");
