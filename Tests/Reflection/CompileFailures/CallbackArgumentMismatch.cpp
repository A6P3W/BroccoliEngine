#include "ReflectionGenerator.h"

struct FInvalidCallbackArgument {
  void OnValueChanged(float) {}
  EDITOR_PROPERTY(.OnEditorChanged = ^^FInvalidCallbackArgument::OnValueChanged)
  int Value = 0;
};

const FClass InvalidClass =
    ReflectionGenerator::MakeClass<FInvalidCallbackArgument>("InvalidCallback");
