#pragma once

#include <array>
#include <concepts>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "AutomationTypes.h"
#include "Detail/AutomationRegistrationBridge.h"
#include "UMath.h"

template <class T, class TEnable = void>
struct TAutomationJsonConverter;

template <class T>
concept AutomationJsonReadable = requires(const nlohmann::json& Json, T& Value) {
  { TAutomationJsonConverter<T>::FromJson(Json, Value) } -> std::convertible_to<bool>;
  { TAutomationJsonConverter<T>::GetSchema() } -> std::convertible_to<nlohmann::json>;
};

template <class T>
concept AutomationJsonWritable = requires(const std::remove_cvref_t<T>& Value) {
  {
    TAutomationJsonConverter<std::remove_cvref_t<T>>::ToJson(Value)
  } -> std::convertible_to<nlohmann::json>;
};

template <>
struct TAutomationJsonConverter<bool> {
  static bool FromJson(const nlohmann::json& Json, bool& OutValue) {
    if (!Json.is_boolean()) {
      return false;
    }
    OutValue = Json.get<bool>();
    return true;
  }

  static nlohmann::json ToJson(bool Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "boolean"}}; }
};

template <std::integral T>
  requires(!std::same_as<T, bool>)
struct TAutomationJsonConverter<T> {
  static bool FromJson(const nlohmann::json& Json, T& OutValue) {
    try {
      if (!Json.is_number_integer() && !Json.is_number_unsigned()) {
        return false;
      }
      OutValue = Json.get<T>();
      return true;
    } catch (const nlohmann::json::exception&) {
      return false;
    }
  }

  static nlohmann::json ToJson(T Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "integer"}}; }
};

template <std::floating_point T>
struct TAutomationJsonConverter<T> {
  static bool FromJson(const nlohmann::json& Json, T& OutValue) {
    try {
      if (!Json.is_number()) {
        return false;
      }
      OutValue = Json.get<T>();
      return true;
    } catch (const nlohmann::json::exception&) {
      return false;
    }
  }

  static nlohmann::json ToJson(T Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "number"}}; }
};

template <>
struct TAutomationJsonConverter<std::string> {
  static bool FromJson(const nlohmann::json& Json, std::string& OutValue) {
    if (!Json.is_string()) {
      return false;
    }
    OutValue = Json.get<std::string>();
    return true;
  }

  static nlohmann::json ToJson(const std::string& Value) { return Value; }
  static nlohmann::json GetSchema() { return {{"type", "string"}}; }
};

template <class T>
  requires AutomationJsonReadable<T> && AutomationJsonWritable<T>
struct TAutomationJsonConverter<std::vector<T>> {
  static bool FromJson(const nlohmann::json& Json, std::vector<T>& OutValue) {
    if (!Json.is_array()) {
      return false;
    }
    std::vector<T> Values;
    Values.reserve(Json.size());
    for (const nlohmann::json& Element : Json) {
      T Value{};
      if (!TAutomationJsonConverter<T>::FromJson(Element, Value)) {
        return false;
      }
      Values.push_back(std::move(Value));
    }
    OutValue = std::move(Values);
    return true;
  }

  static nlohmann::json ToJson(const std::vector<T>& Value) {
    nlohmann::json Result = nlohmann::json::array();
    for (const T& Element : Value) {
      Result.push_back(TAutomationJsonConverter<T>::ToJson(Element));
    }
    return Result;
  }

  static nlohmann::json GetSchema() {
    return {{"type", "array"}, {"items", TAutomationJsonConverter<T>::GetSchema()}};
  }
};

template <class T>
  requires AutomationJsonReadable<T> && AutomationJsonWritable<T>
struct TAutomationJsonConverter<std::optional<T>> {
  static bool FromJson(const nlohmann::json& Json, std::optional<T>& OutValue) {
    if (Json.is_null()) {
      OutValue.reset();
      return true;
    }
    T Value{};
    if (!TAutomationJsonConverter<T>::FromJson(Json, Value)) {
      return false;
    }
    OutValue = std::move(Value);
    return true;
  }

  static nlohmann::json ToJson(const std::optional<T>& Value) {
    return Value ? TAutomationJsonConverter<T>::ToJson(*Value) : nlohmann::json(nullptr);
  }

  static nlohmann::json GetSchema() { return TAutomationJsonConverter<T>::GetSchema(); }
};

template <>
struct TAutomationJsonConverter<FVector2D> {
  static bool FromJson(const nlohmann::json& Json, FVector2D& OutValue) {
    if (!Json.is_object() || !Json.contains("x") || !Json.contains("y")) {
      return false;
    }
    return TAutomationJsonConverter<float>::FromJson(Json["x"], OutValue.X) &&
           TAutomationJsonConverter<float>::FromJson(Json["y"], OutValue.Y);
  }

  static nlohmann::json ToJson(const FVector2D& Value) { return {{"x", Value.X}, {"y", Value.Y}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties",
         {{"x", TAutomationJsonConverter<float>::GetSchema()},
          {"y", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"x", "y"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<FRotator> {
  static bool FromJson(const nlohmann::json& Json, FRotator& OutValue) {
    return Json.is_object() && Json.contains("rotation") &&
           TAutomationJsonConverter<float>::FromJson(Json["rotation"], OutValue.Rotation);
  }

  static nlohmann::json ToJson(const FRotator& Value) { return {{"rotation", Value.Rotation}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties", {{"rotation", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"rotation"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<FScale> {
  static bool FromJson(const nlohmann::json& Json, FScale& OutValue) {
    return Json.is_object() && Json.contains("scale") &&
           TAutomationJsonConverter<float>::FromJson(Json["scale"], OutValue.Scale);
  }

  static nlohmann::json ToJson(const FScale& Value) { return {{"scale", Value.Scale}}; }

  static nlohmann::json GetSchema() {
    return {
        {"type", "object"},
        {"properties", {{"scale", TAutomationJsonConverter<float>::GetSchema()}}},
        {"required", {"scale"}},
        {"additionalProperties", false}
    };
  }
};

template <>
struct TAutomationJsonConverter<nlohmann::json> {
  static nlohmann::json ToJson(const nlohmann::json& Value) { return Value; }
};

struct FAutomationParameterMetadata {
  std::string Name;
  std::string Description;
};

namespace BroccoliAutomationDetail {

inline FAutomationParameterMetadata MakeParameterMetadata(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description)};
}

template <class... TParameters>
  requires(std::same_as<std::remove_cvref_t<TParameters>, FAutomationParameterMetadata> && ...)
constexpr std::array<FAutomationParameterMetadata, sizeof...(TParameters)>
MakeParameterMetadataList(TParameters&&... Parameters) {
  return {{std::forward<TParameters>(Parameters)...}};
}

}  // namespace BroccoliAutomationDetail

#define BROCCOLI_AUTOMATION_PARAM_IMPL(Name, Description) \
  BroccoliAutomationDetail::MakeParameterMetadata(Name, Description)

#define BROCCOLI_AUTOMATION_PARAMS_IMPL(...) \
  (BroccoliAutomationDetail::MakeParameterMetadataList(__VA_ARGS__))

#include "Actor.h"
#include "ActorComponent.h"

namespace BroccoliAutomationDetail {

template <class T>
using TArgumentStorage = std::remove_cvref_t<T>;

template <class T>
struct TMethodTraits;

template <class TReturn, class TOwner, class... TArguments>
struct TMethodTraits<TReturn (TOwner::*)(TArguments...)> {
  using OwnerType = TOwner;
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArguments...>;
  static constexpr size_t ArgumentCount = sizeof...(TArguments);
};

template <class TReturn, class TOwner, class... TArguments>
struct TMethodTraits<TReturn (TOwner::*)(TArguments...) const> {
  using OwnerType = TOwner;
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArguments...>;
  static constexpr size_t ArgumentCount = sizeof...(TArguments);
};

template <class TReturn, class TOwner, class... TArguments>
struct TMethodTraits<TReturn (TOwner::*)(TArguments...) noexcept>
    : TMethodTraits<TReturn (TOwner::*)(TArguments...)> {};

template <class TReturn, class TOwner, class... TArguments>
struct TMethodTraits<TReturn (TOwner::*)(TArguments...) const noexcept>
    : TMethodTraits<TReturn (TOwner::*)(TArguments...) const> {};

template <class T>
concept AutomationMemberFunction = std::is_member_function_pointer_v<T>;

template <class T>
concept AutomationOwner = std::derived_from<T, AActor> || std::derived_from<T, MActorComponent>;

template <class TMethod, size_t... TIndices>
  requires AutomationMemberFunction<TMethod>
consteval bool AreMethodArgumentsJsonReadable(std::index_sequence<TIndices...>) {
  using TTraits = TMethodTraits<TMethod>;
  return (
      AutomationJsonReadable<
          TArgumentStorage<std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>> &&
      ...
  );
}

template <class TMethod>
concept AutomationMethodArgumentsJsonReadable =
    AutomationMemberFunction<TMethod> &&
    AreMethodArgumentsJsonReadable<TMethod>(
        std::make_index_sequence<TMethodTraits<TMethod>::ArgumentCount>{}
    );

template <class TResultAdapter, class TOwner, class TResult>
concept AutomationResultAdapter =
    requires(TResultAdapter& Adapter, TOwner& Owner, TResult&& Result) {
      {
        std::invoke(Adapter, Owner, std::forward<TResult>(Result))
      } -> std::convertible_to<nlohmann::json>;
    } || requires(TResultAdapter& Adapter, const TOwner& Owner, TResult&& Result) {
      {
        std::invoke(Adapter, Owner, std::forward<TResult>(Result))
      } -> std::convertible_to<nlohmann::json>;
    } || requires(TResultAdapter& Adapter, TResult&& Result) {
      {
        std::invoke(Adapter, std::forward<TResult>(Result))
      } -> std::convertible_to<nlohmann::json>;
    };

template <class TMethod, class TResultAdapter>
concept AutomationMethodResultCompatible =
    AutomationMemberFunction<TMethod> &&
    ((std::same_as<std::remove_cvref_t<TResultAdapter>, std::nullptr_t> &&
      (std::is_void_v<typename TMethodTraits<TMethod>::ReturnType> ||
       AutomationJsonWritable<typename TMethodTraits<TMethod>::ReturnType>)) ||
     (!std::same_as<std::remove_cvref_t<TResultAdapter>, std::nullptr_t> &&
      !std::is_void_v<typename TMethodTraits<TMethod>::ReturnType> &&
      AutomationResultAdapter<
          TResultAdapter,
          typename TMethodTraits<TMethod>::OwnerType,
          typename TMethodTraits<TMethod>::ReturnType>));

template <class TValue>
TValue ReadArgument(
    const nlohmann::json& Arguments, const FAutomationParameterMetadata& Parameter
) {
  TValue Value{};
  const auto Iterator = Arguments.find(Parameter.Name);
  if (Iterator == Arguments.end()) {
    throw std::runtime_error("Missing automation argument: " + Parameter.Name);
  }
  if (!TAutomationJsonConverter<TValue>::FromJson(*Iterator, Value)) {
    throw std::runtime_error("Invalid automation argument: " + Parameter.Name);
  }
  return Value;
}

template <class TMethod, size_t... TIndices>
auto ReadArguments(
    const nlohmann::json& Arguments,
    std::span<const FAutomationParameterMetadata> Parameters,
    std::index_sequence<TIndices...>
) {
  using TTraits = TMethodTraits<TMethod>;
  return std::tuple<
      TArgumentStorage<std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>...>{
      ReadArgument<
          TArgumentStorage<std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>>(
          Arguments, Parameters[TIndices]
      )...
  };
}

template <class TReturn>
nlohmann::json ConvertReturnValue(TReturn&& Result) {
  return TAutomationJsonConverter<std::remove_cvref_t<TReturn>>::ToJson(
      std::forward<TReturn>(Result)
  );
}

template <class TMethod, size_t... TIndices>
nlohmann::json MakeInputSchema(
    std::span<const FAutomationParameterMetadata> Parameters, std::index_sequence<TIndices...>
) {
  using TTraits = TMethodTraits<TMethod>;
  nlohmann::json Properties = nlohmann::json::object();
  nlohmann::json Required = nlohmann::json::array();
  (
      [&] {
        using TArgument =
            TArgumentStorage<std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>;
        const auto& Parameter = Parameters[TIndices];
        Properties[Parameter.Name] = TAutomationJsonConverter<TArgument>::GetSchema();
        if (!Parameter.Description.empty()) {
          Properties[Parameter.Name]["description"] = Parameter.Description;
        }
        Required.push_back(Parameter.Name);
      }(),
      ...);

  nlohmann::json Schema = {
      {"type", "object"}, {"properties", std::move(Properties)}, {"additionalProperties", false}
  };
  if (!Required.empty()) {
    Schema["required"] = std::move(Required);
  }
  return Schema;
}

template <class TOwner, class TResultAdapter, class TReturn>
nlohmann::json InvokeResultAdapter(TOwner& Owner, TResultAdapter& ResultAdapter, TReturn&& Result) {
  if constexpr (std::invocable<TResultAdapter&, TOwner&, TReturn>) {
    return nlohmann::json(std::invoke(ResultAdapter, Owner, std::forward<TReturn>(Result)));
  } else if constexpr (std::invocable<TResultAdapter&, const TOwner&, TReturn>) {
    return nlohmann::json(
        std::invoke(ResultAdapter, std::as_const(Owner), std::forward<TReturn>(Result))
    );
  } else if constexpr (std::invocable<TResultAdapter&, TReturn>) {
    return nlohmann::json(std::invoke(ResultAdapter, std::forward<TReturn>(Result)));
  } else {
    static_assert(std::invocable<TResultAdapter&, TReturn>, "Invalid automation result adapter.");
  }
}

template <class TOwner, class TMethod, class TResultAdapter>
nlohmann::json InvokeMethod(
    TOwner& Owner,
    TMethod Method,
    const nlohmann::json& Arguments,
    std::span<const FAutomationParameterMetadata> Parameters,
    TResultAdapter& ResultAdapter
) {
  using TTraits = TMethodTraits<TMethod>;
  using TReturn = typename TTraits::ReturnType;
  auto Values = ReadArguments<TMethod>(
      Arguments, Parameters, std::make_index_sequence<TTraits::ArgumentCount>{}
  );
  if constexpr (std::is_void_v<TReturn>) {
    std::apply(
        [&Owner, Method](auto&&... MethodArguments) {
          std::invoke(Method, Owner, std::forward<decltype(MethodArguments)>(MethodArguments)...);
        },
        std::move(Values)
    );
    return nlohmann::json{{"success", true}};
  } else {
    decltype(auto) Result = std::apply(
        [&Owner, Method](auto&&... MethodArguments) -> decltype(auto) {
          return std::invoke(
              Method, Owner, std::forward<decltype(MethodArguments)>(MethodArguments)...
          );
        },
        std::move(Values)
    );
    if constexpr (std::same_as<std::remove_cvref_t<TResultAdapter>, std::nullptr_t>) {
      return ConvertReturnValue(std::forward<decltype(Result)>(Result));
    } else {
      return InvokeResultAdapter(Owner, ResultAdapter, std::forward<decltype(Result)>(Result));
    }
  }
}

template <AutomationMemberFunction TMethod, size_t N, class TResultAdapter>
  requires AutomationOwner<typename TMethodTraits<TMethod>::OwnerType> &&
           AutomationMethodArgumentsJsonReadable<TMethod> &&
           AutomationMethodResultCompatible<TMethod, TResultAdapter> &&
           (N == TMethodTraits<TMethod>::ArgumentCount)
void RegisterMethod(
    FAutomationRegistrationContext& Context,
    std::string Name,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    std::array<FAutomationParameterMetadata, N> Parameters,
    TResultAdapter ResultAdapter
) {
  using TTraits = TMethodTraits<TMethod>;
  using TOwner = typename TTraits::OwnerType;
  nlohmann::json InputSchema = MakeInputSchema<TMethod>(
      std::span<const FAutomationParameterMetadata>(Parameters),
      std::make_index_sequence<TTraits::ArgumentCount>{}
  );

  if constexpr (std::is_base_of_v<AActor, TOwner>) {
    FAutomationActorHandler Handler =
        [Method, Parameters = std::move(Parameters), ResultAdapter = std::move(ResultAdapter)](
            AActor& Actor, const nlohmann::json& Arguments
        ) mutable {
          auto* TypedActor = dynamic_cast<TOwner*>(&Actor);
          if (!TypedActor) {
            throw std::runtime_error("Automation actor type mismatch.");
          }
          return InvokeMethod(
              *TypedActor,
              Method,
              Arguments,
              std::span<const FAutomationParameterMetadata>(Parameters),
              ResultAdapter
          );
        };
    Context.RegisterActorMethod(
        TOwner::StaticClassName(),
        std::move(Name),
        std::move(Description),
        std::move(InputSchema),
        Permission,
        std::move(Handler)
    );
  } else {
    FAutomationComponentHandler Handler =
        [Method, Parameters = std::move(Parameters), ResultAdapter = std::move(ResultAdapter)](
            MActorComponent& Component, const nlohmann::json& Arguments
        ) mutable {
          auto* TypedComponent = dynamic_cast<TOwner*>(&Component);
          if (!TypedComponent) {
            throw std::runtime_error("Automation component type mismatch.");
          }
          return InvokeMethod(
              *TypedComponent,
              Method,
              Arguments,
              std::span<const FAutomationParameterMetadata>(Parameters),
              ResultAdapter
          );
        };
    Context.RegisterComponentMethod(
        TOwner::StaticComponentClassName(),
        std::move(Name),
        std::move(Description),
        std::move(InputSchema),
        Permission,
        std::move(Handler)
    );
  }
}

template <AutomationMemberFunction TMethod>
  requires AutomationOwner<typename TMethodTraits<TMethod>::OwnerType> &&
           AutomationMethodArgumentsJsonReadable<TMethod> &&
           AutomationMethodResultCompatible<TMethod, std::nullptr_t>
void RegisterMethod(
    FAutomationRegistrationContext& Context,
    std::string Name,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method
) {
  RegisterMethod(
      Context,
      std::move(Name),
      std::move(Description),
      Permission,
      Method,
      std::array<FAutomationParameterMetadata, 0>{},
      nullptr
  );
}

template <AutomationMemberFunction TMethod, size_t N>
  requires AutomationOwner<typename TMethodTraits<TMethod>::OwnerType> &&
           AutomationMethodArgumentsJsonReadable<TMethod> &&
           AutomationMethodResultCompatible<TMethod, std::nullptr_t> &&
           (N == TMethodTraits<TMethod>::ArgumentCount)
void RegisterMethod(
    FAutomationRegistrationContext& Context,
    std::string Name,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    std::array<FAutomationParameterMetadata, N> Parameters
) {
  RegisterMethod(
      Context,
      std::move(Name),
      std::move(Description),
      Permission,
      Method,
      std::move(Parameters),
      nullptr
  );
}

}  // namespace BroccoliAutomationDetail

#define BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right) Left##Right
#define BROCCOLI_AUTOMATION_CONCAT(Left, Right) BROCCOLI_AUTOMATION_CONCAT_IMPL(Left, Right)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(Counter, ...) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, __VA_ARGS__)

#define BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL_EXPANDED(Counter, ...)         \
  namespace {                                                                   \
  void BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter)(    \
      BroccoliAutomationDetail::FAutomationRegistrationContext & Context        \
  ) {                                                                           \
    BroccoliAutomationDetail::RegisterMethod(Context, __VA_ARGS__);             \
  }                                                                             \
  const BroccoliAutomationDetail::FAutomationRegistrationToken                  \
      BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationAutoRegister_, Counter)(     \
          &BROCCOLI_AUTOMATION_CONCAT(BroccoliAutomationRegistration_, Counter) \
      );                                                                        \
  }

#define REGISTER_AUTOMATION_METHOD(...) \
  BROCCOLI_REGISTER_AUTOMATION_METHOD_IMPL(__COUNTER__, __VA_ARGS__)

#define AUTOMATION_PARAMS(...) BROCCOLI_AUTOMATION_PARAMS_IMPL(__VA_ARGS__)
#define AUTOMATION_PARAM(Name, Description) BROCCOLI_AUTOMATION_PARAM_IMPL(Name, Description)
