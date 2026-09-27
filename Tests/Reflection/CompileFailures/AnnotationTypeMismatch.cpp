#include "ReflectionGenerator.h"

struct FInvalidAnnotationType {
  EDITOR_PROPERTY()
  double Value = 0.0;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FInvalidAnnotationType>("InvalidType");
