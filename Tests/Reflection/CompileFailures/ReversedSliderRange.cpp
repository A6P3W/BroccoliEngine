#include "ReflectionGenerator.h"

struct FReversedSliderRange {
  EDITOR_PROPERTY(.SliderMin = 10, .SliderMax = 1)
  int Value = 0;
};

const FClass InvalidClass = ReflectionGenerator::MakeClass<FReversedSliderRange>("ReversedSlider");
