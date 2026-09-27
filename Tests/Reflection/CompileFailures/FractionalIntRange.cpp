#include "ReflectionGenerator.h"

struct FFractionalIntRange {
  EDITOR_PROPERTY(.Min = 0.5)
  int Value = 0;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FFractionalIntRange>("FractionalInt");
