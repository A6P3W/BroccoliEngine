#include "ReflectionGenerator.h"

struct FPrivateDirectAccess {
 private:
  EDITOR_PROPERTY()
  int Value = 0;
};

int ReadDirectly(FPrivateDirectAccess& Object) { return Object.Value; }
