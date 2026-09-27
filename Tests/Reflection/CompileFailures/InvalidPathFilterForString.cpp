#include "ReflectionGenerator.h"

struct FInvalidPathFilterForString {
  EDITOR_PROPERTY(.PathFilter = EPathFilter::Image)
  std::string Value;
};

const FClass InvalidClass =
    ReflectionGenerator::MakeClass<FInvalidPathFilterForString>("InvalidPathFilter");
