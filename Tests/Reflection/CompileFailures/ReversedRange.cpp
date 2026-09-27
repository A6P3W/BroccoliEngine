#include "ReflectionGenerator.h"

struct FInvalidRange {
  EDITOR_PROPERTY(.Min = 10, .Max = 1)
  int Value = 0;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FInvalidRange>("InvalidRange");
