#include "ReflectionGenerator.h"

struct FInvalidAnnotationType {
  [[= FIntEditorProperty{}]] bool Value = false;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FInvalidAnnotationType>("InvalidType");
