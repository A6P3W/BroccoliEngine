#pragma once

#include <algorithm>
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

struct FEditorProperty {
  std::meta::info OnChanged{};
};

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

struct FFloatEditorProperty {
  FEditorProperty Base{};
  TAnnotationOptional<float> Min, Max, SliderMin, SliderMax;
};
struct FIntEditorProperty {
  FEditorProperty Base{};
  TAnnotationOptional<int> Min, Max, SliderMin, SliderMax;
};
struct FBoolEditorProperty {
  FEditorProperty Base{};
};
struct FStringEditorProperty {
  FEditorProperty Base{};
  TAnnotationOptional<std::size_t> MaxLength;
};
struct FVector2DEditorProperty {
  FEditorProperty Base{};
};
struct FVector3DEditorProperty {
  FEditorProperty Base{};
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

template <std::meta::info Member>
consteval int AnnotationCount() {
  return int(HasAnnotation<Member, FBoolEditorProperty>()) +
         int(HasAnnotation<Member, FIntEditorProperty>()) +
         int(HasAnnotation<Member, FFloatEditorProperty>()) +
         int(HasAnnotation<Member, FStringEditorProperty>()) +
         int(HasAnnotation<Member, FVector2DEditorProperty>()) +
         int(HasAnnotation<Member, FVector3DEditorProperty>());
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
    constexpr auto Annotation = GetAnnotation<Member, FIntEditorProperty>();
    if constexpr (Annotation.Min.Present) {
      constexpr int Min = Annotation.Min.Value;
      NewValue = std::max(NewValue, Min);
    }
    if constexpr (Annotation.Max.Present) {
      constexpr int Max = Annotation.Max.Value;
      NewValue = std::min(NewValue, Max);
    }
  } else if constexpr (std::is_same_v<V, float>) {
    constexpr auto Annotation = GetAnnotation<Member, FFloatEditorProperty>();
    if (!std::isfinite(Input)) return Reject<Member>("non-finite float");
    if constexpr (Annotation.Min.Present) {
      constexpr float Min = Annotation.Min.Value;
      NewValue = std::max(NewValue, Min);
    }
    if constexpr (Annotation.Max.Present) {
      constexpr float Max = Annotation.Max.Value;
      NewValue = std::min(NewValue, Max);
    }
  } else if constexpr (std::is_same_v<V, std::string>) {
    constexpr auto Annotation = GetAnnotation<Member, FStringEditorProperty>();
    constexpr std::size_t MaxLength =
        Annotation.MaxLength.value_or(std::numeric_limits<std::size_t>::max());
    if (!IsValidUnicodeScalarString(Input, MaxLength))
      return Reject<Member>("invalid UTF-8 or length limit");
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
    constexpr std::meta::info Callback = []() consteval {
      if constexpr (std::is_same_v<V, bool>)
        return GetAnnotation<Member, FBoolEditorProperty>().Base.OnChanged;
      else if constexpr (std::is_same_v<V, int>)
        return GetAnnotation<Member, FIntEditorProperty>().Base.OnChanged;
      else if constexpr (std::is_same_v<V, float>)
        return GetAnnotation<Member, FFloatEditorProperty>().Base.OnChanged;
      else if constexpr (std::is_same_v<V, std::string>)
        return GetAnnotation<Member, FStringEditorProperty>().Base.OnChanged;
      else if constexpr (std::is_same_v<V, FVector2D>)
        return GetAnnotation<Member, FVector2DEditorProperty>().Base.OnChanged;
      else
        return GetAnnotation<Member, FVector3DEditorProperty>().Base.OnChanged;
    }();
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

template <class T, std::meta::info Member, class V, class Annotation>
void AppendTyped(FClass& Class, EPropertyType Type) {
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(std::declval<T>().[:Member:])>, V>,
      "Editor annotation does not match member type"
  );
  constexpr auto Value = GetAnnotation<Member, Annotation>();
  constexpr std::meta::info Callback = Value.Base.OnChanged;
  if constexpr (Callback != std::meta::info{}) {
    static_assert(
        std::is_same_v<decltype(&[:Callback:]), void (T::*)(V)>,
        "OnChanged must be void(T OldValue) on the declaring class"
    );
  }
  FProperty Property;
  Property.Name = std::string(std::meta::identifier_of(Member));
  Property.Type = Type;
  Property.Get = &Get<T, Member, V>;
  Property.Set = &Set<T, Member, V>;
  if constexpr (std::is_same_v<V, int>) {
    static_assert(!Value.Min || !Value.Max || *Value.Min <= *Value.Max, "Reversed Min/Max");
    static_assert(
        !Value.SliderMin || !Value.SliderMax || *Value.SliderMin <= *Value.SliderMax,
        "Reversed SliderMin/SliderMax"
    );
    if constexpr (Value.Min.Present) {
      constexpr int Min = Value.Min.Value;
      Property.EditorMetadata.IntMin = Min;
    }
    if constexpr (Value.Max.Present) {
      constexpr int Max = Value.Max.Value;
      Property.EditorMetadata.IntMax = Max;
    }
    if constexpr (Value.SliderMin.Present) {
      constexpr int SliderMin = Value.SliderMin.Value;
      Property.EditorMetadata.IntSliderMin = SliderMin;
    }
    if constexpr (Value.SliderMax.Present) {
      constexpr int SliderMax = Value.SliderMax.Value;
      Property.EditorMetadata.IntSliderMax = SliderMax;
    }
  } else if constexpr (std::is_same_v<V, float>) {
    static_assert(!Value.Min || std::isfinite(*Value.Min), "Non-finite Min");
    static_assert(!Value.Max || std::isfinite(*Value.Max), "Non-finite Max");
    static_assert(!Value.SliderMin || std::isfinite(*Value.SliderMin), "Non-finite SliderMin");
    static_assert(!Value.SliderMax || std::isfinite(*Value.SliderMax), "Non-finite SliderMax");
    static_assert(!Value.Min || !Value.Max || *Value.Min <= *Value.Max, "Reversed Min/Max");
    static_assert(
        !Value.SliderMin || !Value.SliderMax || *Value.SliderMin <= *Value.SliderMax,
        "Reversed SliderMin/SliderMax"
    );
    if constexpr (Value.Min.Present) {
      constexpr float Min = Value.Min.Value;
      Property.EditorMetadata.FloatMin = Min;
    }
    if constexpr (Value.Max.Present) {
      constexpr float Max = Value.Max.Value;
      Property.EditorMetadata.FloatMax = Max;
    }
    if constexpr (Value.SliderMin.Present) {
      constexpr float SliderMin = Value.SliderMin.Value;
      Property.EditorMetadata.FloatSliderMin = SliderMin;
    }
    if constexpr (Value.SliderMax.Present) {
      constexpr float SliderMax = Value.SliderMax.Value;
      Property.EditorMetadata.FloatSliderMax = SliderMax;
    }
  } else if constexpr (std::is_same_v<V, std::string>) {
    if constexpr (Value.MaxLength.Present) {
      constexpr std::size_t MaxLength = Value.MaxLength.Value;
      Property.EditorMetadata.MaxLength = MaxLength;
    }
  }
  Class.OwnProperties.push_back(std::move(Property));
}

template <class T, std::meta::info Member>
void Append(FClass& Class) {
  constexpr int Count = AnnotationCount<Member>();
  static_assert(Count <= 1, "Multiple editor annotations on one member");
  if constexpr (Count > 0) {
    static_assert(std::meta::is_public(Member), "Editor property must be public");
    if constexpr (HasAnnotation<Member, FBoolEditorProperty>())
      AppendTyped<T, Member, bool, FBoolEditorProperty>(Class, EPropertyType::Bool);
    else if constexpr (HasAnnotation<Member, FIntEditorProperty>())
      AppendTyped<T, Member, int, FIntEditorProperty>(Class, EPropertyType::Int);
    else if constexpr (HasAnnotation<Member, FFloatEditorProperty>())
      AppendTyped<T, Member, float, FFloatEditorProperty>(Class, EPropertyType::Float);
    else if constexpr (HasAnnotation<Member, FStringEditorProperty>())
      AppendTyped<T, Member, std::string, FStringEditorProperty>(Class, EPropertyType::String);
    else if constexpr (HasAnnotation<Member, FVector2DEditorProperty>())
      AppendTyped<T, Member, FVector2D, FVector2DEditorProperty>(Class, EPropertyType::Vector2D);
    else if constexpr (HasAnnotation<Member, FVector3DEditorProperty>())
      AppendTyped<T, Member, FVector3D, FVector3DEditorProperty>(Class, EPropertyType::Vector3D);
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
