#include "ReflectionGenerator.h"

struct FInvalidCallbackArgument {
  void OnValueChanged(float) {}
  [[= FIntEditorProperty{
      .Base = {.OnChanged = ^^FInvalidCallbackArgument::OnValueChanged}
  }]] int Value = 0;
};

const FClass InvalidClass =
    ReflectionGenerator::MakeClass<FInvalidCallbackArgument>("InvalidCallback");
