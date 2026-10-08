#include <meta>

#include "AutomationAnnotations.h"
#include "FunctionReflection.h"

class FControlReflectionProbeBase {
 private:
  CONTROL_METHOD(.Name = "base_method", .Description = "Base method.")
  void BaseMethod() {}
};

class FControlReflectionProbe final : public FControlReflectionProbeBase {
 private:
  static int Adapt(int Result) { return Result; }

 protected:
  CONTROL_METHOD(
      .Name = "derived_method",
      .Description = "Derived method.",
      .ResultAdapter = ^^FControlReflectionProbe::Adapt
  )
  CONTROL_PARAMETER(.Index = 0, .Name = "first", .Description = "First value.")
  CONTROL_PARAMETER(.Index = 1, .Name = "second", .Description = "Second value.")
  int DerivedMethod(int First, int Second) { return First + Second; }
};

struct FControlReflectionVisitor {
  int Count = 0;

  template <std::meta::info Function>
  void operator()() {
    constexpr auto Annotation = [] consteval {
      auto Values = std::meta::annotations_of_with_type(Function, ^^FControlMethodAnnotation);
      return std::meta::extract<FControlMethodAnnotation>(Values[0]);
    }();
    static_assert(Annotation.Name.Size > 0);
    auto Method = &[:Function:];
    (void)Method;
    ++Count;
  }
};

int main() {
  FControlReflectionVisitor Visitor;
  FunctionReflection::ForEachAnnotatedFunction<FControlReflectionProbe, FControlMethodAnnotation>(
      Visitor
  );
  return Visitor.Count == 2 ? 0 : 1;
}
