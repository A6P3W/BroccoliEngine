#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <meta>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "ActorRegistry.h"
#include "Log.h"
#include "Reflection.h"

#ifdef __INTELLISENSE__
#define EDITOR_PROPERTY(...)
#else
#define EDITOR_PROPERTY(...) [[= FEditorPropertyAnnotation{__VA_ARGS__}]]
#endif

// Annotation values must be structural; std::optional is not structural in GCC 16.
template <class T>
struct TAnnotationOptional {
  T Value{};
  bool Present = false;

  constexpr TAnnotationOptional() = default;
  constexpr TAnnotationOptional(T InValue) : Value(InValue), Present(true) {}
  constexpr explicit operator bool() const { return Present; }
  constexpr T operator*() const { return Value; }
  constexpr T value_or(T Fallback) const { return Present ? Value : Fallback; }
};

struct FAnnotationNumber {
  double Value = 0.0;
  bool Present = false;

  constexpr FAnnotationNumber() = default;

  template <class T>
    requires(std::is_arithmetic_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>)
  constexpr FAnnotationNumber(T InValue) : Value(static_cast<double>(InValue)), Present(true) {}

  constexpr explicit operator bool() const { return Present; }
  constexpr double operator*() const { return Value; }
};

struct FEditorPropertyAnnotation {
  std::meta::info OnEditorChanged{};
  FAnnotationNumber Min, Max, SliderMin, SliderMax;
  TAnnotationOptional<std::size_t> MaxLength;
  EPathFilter PathFilter = EPathFilter::AnyFile;
  char EnumOptions[64] = {};
};

namespace ReflectionGenerator {
template <std::meta::info Member, class Annotation>
consteval bool HasAnnotation() {
  static_assert(
      std::meta::annotations_of_with_type(Member, ^^Annotation).size() <= 1,
      "Multiple editor annotations of one type"
  );
  return !std::meta::annotations_of_with_type(Member, ^^Annotation).empty();
}

template <std::meta::info Member, class Annotation>
consteval Annotation GetAnnotation() {
  static_assert(std::meta::annotations_of_with_type(Member, ^^Annotation).size() == 1);
  auto Annotations = std::meta::annotations_of_with_type(Member, ^^Annotation);
  return std::meta::extract<Annotation>(Annotations[0]);
}

template <class T, std::size_t Index>
consteval std::meta::info MemberAt() {
  return std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())[Index];
}

template <class T, std::meta::info Member, class V>
FPropertyValue Get(const void* Object) {
  if constexpr (std::is_base_of_v<AActor, T>) {
    return dynamic_cast<const T*>(static_cast<const AActor*>(Object))->[:Member:];
  } else {
    return static_cast<const T*>(Object)->[:Member:];
  }
}

template <std::meta::info Member>
bool Reject(const char* Reason) {
  M_LOG(Warning, "Reflection property '{}' rejected: {}", std::meta::identifier_of(Member), Reason);
  return false;
}

template <class T, std::meta::info Member, class V>
bool Set(void* Object, const FPropertyValue& Value) {
  if (Object == nullptr) return Reject<Member>("null object");
  if (!std::holds_alternative<V>(Value)) return Reject<Member>("value type mismatch");
  const V& Input = std::get<V>(Value);
  V NewValue = Input;
  if constexpr (std::is_same_v<V, int>) {
    constexpr auto Annotation = GetAnnotation<Member, FEditorPropertyAnnotation>();
    if constexpr (Annotation.Min.Present) {
      constexpr int Min = static_cast<int>(Annotation.Min.Value);
      NewValue = std::max(NewValue, Min);
    }
    if constexpr (Annotation.Max.Present) {
      constexpr int Max = static_cast<int>(Annotation.Max.Value);
      NewValue = std::min(NewValue, Max);
    }
  } else if constexpr (std::is_same_v<V, float>) {
    constexpr auto Annotation = GetAnnotation<Member, FEditorPropertyAnnotation>();
    if (!std::isfinite(Input)) return Reject<Member>("non-finite float");
    if constexpr (Annotation.Min.Present) {
      constexpr float Min = static_cast<float>(Annotation.Min.Value);
      NewValue = std::max(NewValue, Min);
    }
    if constexpr (Annotation.Max.Present) {
      constexpr float Max = static_cast<float>(Annotation.Max.Value);
      NewValue = std::min(NewValue, Max);
    }
  } else if constexpr (std::is_same_v<V, std::string>) {
    constexpr auto Annotation = GetAnnotation<Member, FEditorPropertyAnnotation>();
    constexpr std::size_t MaxLength =
        Annotation.MaxLength.value_or(std::numeric_limits<std::size_t>::max());
    if (!IsValidUnicodeScalarString(Input, MaxLength))
      return Reject<Member>("invalid UTF-8 or length limit");
  } else if constexpr (std::is_same_v<V, FPath>) {
    if (!IsValidUnicodeScalarString(Input.String(), std::numeric_limits<std::size_t>::max()))
      return Reject<Member>("invalid UTF-8 path");
  } else if constexpr (std::is_same_v<V, FVector2D>) {
    if (!std::isfinite(Input.X) || !std::isfinite(Input.Y))
      return Reject<Member>("non-finite vector");
  } else if constexpr (std::is_same_v<V, FVector3D>) {
    if (!std::isfinite(Input.X) || !std::isfinite(Input.Y) || !std::isfinite(Input.Z))
      return Reject<Member>("non-finite vector");
  }

  T* TypedObject = nullptr;
  if constexpr (std::is_base_of_v<AActor, T>) {
    TypedObject = dynamic_cast<T*>(static_cast<AActor*>(Object));
  } else {
    TypedObject = static_cast<T*>(Object);
  }
  if (TypedObject == nullptr) return Reject<Member>("object class mismatch");
  static thread_local const void* ActiveObject = nullptr;
  if (ActiveObject == Object) return Reject<Member>("recursive Set");
  V& Target = TypedObject->[:Member:];
  const V OldValue = Target;
  bool Same = false;
  if constexpr (std::is_same_v<V, FVector2D>) {
    Same = OldValue.X == NewValue.X && OldValue.Y == NewValue.Y;
  } else if constexpr (std::is_same_v<V, FVector3D>) {
    Same = OldValue.X == NewValue.X && OldValue.Y == NewValue.Y && OldValue.Z == NewValue.Z;
  } else {
    Same = OldValue == NewValue;
  }
  if (Same) return true;
  ActiveObject = Object;
  try {
    Target = std::move(NewValue);
    constexpr std::meta::info Callback =
        GetAnnotation<Member, FEditorPropertyAnnotation>().OnEditorChanged;
    if constexpr (Callback != std::meta::info{}) {
      constexpr auto Method = std::meta::extract<void (T::*)(V)>(Callback);
      (TypedObject->*Method)(OldValue);
    }
  } catch (const std::exception& Error) {
    M_LOG(Error, "Reflection Set callback failed: {}", Error.what());
    ActiveObject = nullptr;
    return false;
  } catch (...) {
    M_LOG(Error, "Reflection Set callback failed with an unknown exception.");
    ActiveObject = nullptr;
    return false;
  }
  ActiveObject = nullptr;
  return true;
}

constexpr bool ValidIntNumber(FAnnotationNumber Number) {
  return !Number.Present || (std::isfinite(Number.Value) &&
                             Number.Value >= static_cast<double>(std::numeric_limits<int>::min()) &&
                             Number.Value <= static_cast<double>(std::numeric_limits<int>::max()) &&
                             std::trunc(Number.Value) == Number.Value);
}

constexpr bool ValidFloatNumber(FAnnotationNumber Number) {
  return !Number.Present ||
         (std::isfinite(Number.Value) &&
          Number.Value >= -static_cast<double>(std::numeric_limits<float>::max()) &&
          Number.Value <= static_cast<double>(std::numeric_limits<float>::max()));
}

template <class T, std::meta::info Member, class V>
void AppendTyped(FClass& Class, EPropertyType Type) {
  constexpr auto Value = GetAnnotation<Member, FEditorPropertyAnnotation>();
  constexpr std::meta::info Callback = Value.OnEditorChanged;
  if constexpr (Callback != std::meta::info{}) {
    using CallbackType = [:std::meta::type_of(Callback):];
    static_assert(
        std::is_same_v<CallbackType, void(V)> && std::meta::parent_of(Callback) == ^^T,
        "OnEditorChanged must be void(T OldValue) on the declaring class"
    );
  }
  FProperty Property;
  static_assert(
      std::is_same_v<V, FPath> || Value.PathFilter == EPathFilter::AnyFile,
      "PathFilter is only valid for FPath properties"
  );
  Property.Name = std::string(std::meta::identifier_of(Member));
  Property.Type = Type;
  Property.Get = &Get<T, Member, V>;
  Property.Set = &Set<T, Member, V>;
  if constexpr (std::is_same_v<V, int>) {
    static_assert(!Value.MaxLength.Present, "MaxLength is only valid for string properties");
    static_assert(
        ValidIntNumber(Value.Min) && ValidIntNumber(Value.Max) && ValidIntNumber(Value.SliderMin) &&
            ValidIntNumber(Value.SliderMax),
        "Integer property metadata must be finite, integral, and within int range"
    );
    static_assert(!Value.Min || !Value.Max || *Value.Min <= *Value.Max, "Reversed Min/Max");
    static_assert(
        !Value.SliderMin || !Value.SliderMax || *Value.SliderMin <= *Value.SliderMax,
        "Reversed SliderMin/SliderMax"
    );
    if constexpr (Value.Min.Present) {
      constexpr int Min = static_cast<int>(Value.Min.Value);
      Property.EditorMetadata.IntMin = Min;
    }
    if constexpr (Value.Max.Present) {
      constexpr int Max = static_cast<int>(Value.Max.Value);
      Property.EditorMetadata.IntMax = Max;
    }
    if constexpr (Value.SliderMin.Present) {
      constexpr int SliderMin = static_cast<int>(Value.SliderMin.Value);
      Property.EditorMetadata.IntSliderMin = SliderMin;
    }
    if constexpr (Value.SliderMax.Present) {
      constexpr int SliderMax = static_cast<int>(Value.SliderMax.Value);
      Property.EditorMetadata.IntSliderMax = SliderMax;
    }
    if constexpr (Value.EnumOptions[0] != '\0') {
      constexpr auto OptionCharacters = []() consteval {
        constexpr auto Annotation = GetAnnotation<Member, FEditorPropertyAnnotation>();
        std::array<char, 64> Characters{};
        for (std::size_t Index = 0; Index < Characters.size(); ++Index)
          Characters[Index] = Annotation.EnumOptions[Index];
        return Characters;
      }();
      std::string Options = OptionCharacters.data();
      std::size_t Start = 0;
      while (Start <= Options.size()) {
        const std::size_t End = Options.find('|', Start);
        Property.EditorMetadata.EnumOptions.push_back(Options.substr(Start, End - Start));
        if (End == std::string::npos) break;
        Start = End + 1;
      }
    }
  } else if constexpr (std::is_same_v<V, float>) {
    static_assert(!Value.MaxLength.Present, "MaxLength is only valid for string properties");
    static_assert(
        ValidFloatNumber(Value.Min) && ValidFloatNumber(Value.Max) &&
            ValidFloatNumber(Value.SliderMin) && ValidFloatNumber(Value.SliderMax),
        "Float property metadata must be finite and within float range"
    );
    static_assert(!Value.Min || !Value.Max || *Value.Min <= *Value.Max, "Reversed Min/Max");
    static_assert(
        !Value.SliderMin || !Value.SliderMax || *Value.SliderMin <= *Value.SliderMax,
        "Reversed SliderMin/SliderMax"
    );
    if constexpr (Value.Min.Present) {
      constexpr float Min = static_cast<float>(Value.Min.Value);
      Property.EditorMetadata.FloatMin = Min;
    }
    if constexpr (Value.Max.Present) {
      constexpr float Max = static_cast<float>(Value.Max.Value);
      Property.EditorMetadata.FloatMax = Max;
    }
    if constexpr (Value.SliderMin.Present) {
      constexpr float SliderMin = static_cast<float>(Value.SliderMin.Value);
      Property.EditorMetadata.FloatSliderMin = SliderMin;
    }
    if constexpr (Value.SliderMax.Present) {
      constexpr float SliderMax = static_cast<float>(Value.SliderMax.Value);
      Property.EditorMetadata.FloatSliderMax = SliderMax;
    }
  } else if constexpr (std::is_same_v<V, std::string>) {
    static_assert(
        !Value.Min.Present && !Value.Max.Present && !Value.SliderMin.Present &&
            !Value.SliderMax.Present,
        "Numeric metadata is only valid for int and float properties"
    );
    if constexpr (Value.MaxLength.Present) {
      constexpr std::size_t MaxLength = Value.MaxLength.Value;
      Property.EditorMetadata.MaxLength = MaxLength;
    }
  } else {
    static_assert(Value.EnumOptions[0] == '\0', "EnumOptions is only valid for int properties");
    static_assert(
        !Value.Min.Present && !Value.Max.Present && !Value.SliderMin.Present &&
            !Value.SliderMax.Present,
        "Numeric metadata is only valid for int and float properties"
    );
    static_assert(!Value.MaxLength.Present, "MaxLength is only valid for string properties");
    if constexpr (std::is_same_v<V, FPath>) {
      constexpr EPathFilter Filter = Value.PathFilter;
      Property.EditorMetadata.PathFilter = Filter;
    }
  }
  Class.OwnProperties.push_back(std::move(Property));
}

template <class T, std::meta::info Member>
void Append(FClass& Class) {
  if constexpr (HasAnnotation<Member, FEditorPropertyAnnotation>()) {
    using V = std::remove_cvref_t<decltype(std::declval<T>().[:Member:])>;
    if constexpr (std::is_same_v<V, bool>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Bool);
    else if constexpr (std::is_same_v<V, int>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Int);
    else if constexpr (std::is_same_v<V, float>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Float);
    else if constexpr (std::is_same_v<V, std::string>)
      AppendTyped<T, Member, V>(Class, EPropertyType::String);
    else if constexpr (std::is_same_v<V, FVector2D>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Vector2D);
    else if constexpr (std::is_same_v<V, FVector3D>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Vector3D);
    else if constexpr (std::is_same_v<V, FPath>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Path);
    else if constexpr (std::is_same_v<V, FColor>)
      AppendTyped<T, Member, V>(Class, EPropertyType::Color);
    else
      static_assert(!std::is_same_v<V, V>, "Unsupported editor property member type");
  }
}

template <class T, std::size_t... Indices>
void AppendAll(FClass& Class, std::index_sequence<Indices...>) {
  (Append<T, MemberAt<T, Indices>()>(Class), ...);
}

template <class T>
FClass MakeClass(std::string Name, const FClass* BaseClass = nullptr) {
  FClass Class{std::move(Name), BaseClass, {}};
  constexpr std::size_t Count =
      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()).size();
  AppendAll<T>(Class, std::make_index_sequence<Count>{});
  return Class;
}

template <class T>
consteval std::meta::info DirectBase() {
  static_assert(
      std::meta::bases_of(^^T, std::meta::access_context::unchecked()).size() == 1,
      "Actor must have exactly one direct base class"
  );
  return std::meta::type_of(std::meta::bases_of(^^T, std::meta::access_context::unchecked())[0]);
}

template <class T>
std::string ClassName() {
  return std::string(std::meta::identifier_of(^^T));
}

template <class T, class Base>
FReflectionRegistry::FToken RegisterClass(
    std::string ModuleOwner, bool RequireActorRegistration = true
) {
  static_assert(std::is_base_of_v<Base, T>);
  if (RequireActorRegistration && !ActorRegistry::GetInstance().Contains(ClassName<T>())) {
    M_LOG(Error, "Actor class '{}' must be registered before Reflection.", ClassName<T>());
    return 0;
  }
  const FClass* BaseClass = nullptr;
  if constexpr (!std::is_same_v<Base, AActor>) {
    BaseClass = FReflectionRegistry::GetInstance().FindClass(ClassName<Base>());
    if (BaseClass == nullptr) {
      M_LOG(Error, "Reflection base class '{}' is not registered.", ClassName<Base>());
      return 0;
    }
  }
  return FReflectionRegistry::GetInstance().Register(
      MakeClass<T>(ClassName<T>(), BaseClass), std::move(ModuleOwner)
  );
}

template <class T>
bool RegisterStaticClass() {
  using Base = [:DirectBase<T>():];
  static_assert(std::is_base_of_v<AActor, Base>);
  if constexpr (!std::is_same_v<Base, AActor>) {
    if (!RegisterStaticClass<Base>()) return false;
  }
  if (FReflectionRegistry::GetInstance().FindClass(ClassName<T>()) != nullptr) return true;
  return RegisterClass<T, Base>("Static", false) != 0;
}
}  // namespace ReflectionGenerator
