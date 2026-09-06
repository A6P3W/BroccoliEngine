#pragma once

#include <array>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "Detail/AutomationJsonConverter.h"
#include "Detail/AutomationRegistrationContext.h"

struct FAutomationParameterMetadata {
  std::string Name;
  std::string Description;
  bool bRequired = true;
};

namespace BroccoliAutomationDetail {

inline FAutomationParameterMetadata MakeParameterMetadata(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description), true};
}

inline FAutomationParameterMetadata MakeOptionalParameterMetadata(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description), false};
}

template <class... TParameters>
  requires(std::same_as<std::remove_cvref_t<TParameters>, FAutomationParameterMetadata> && ...)
constexpr std::array<FAutomationParameterMetadata, sizeof...(TParameters)>
MakeParameterMetadataList(TParameters&&... Parameters) {
  return {{std::forward<TParameters>(Parameters)...}};
}

}  // namespace BroccoliAutomationDetail

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

template <class T>
struct TIsOptional : std::false_type {};

template <class T>
struct TIsOptional<std::optional<T>> : std::true_type {};

template <class TValue>
TValue ReadArgument(
    const nlohmann::json& Arguments, const FAutomationParameterMetadata& Parameter
) {
  static_assert(
      AutomationJsonReadable<TValue>, "Automation argument type must support JSON input."
  );
  TValue Value{};
  const auto Iterator = Arguments.find(Parameter.Name);
  if (Iterator == Arguments.end()) {
    if constexpr (TIsOptional<TValue>::value) {
      return std::nullopt;
    }
    throw std::runtime_error("Missing automation argument: " + Parameter.Name);
  }
  if (!TAutomationJsonConverter<TValue>::FromJson(*Iterator, Value)) {
    throw std::runtime_error("Invalid automation argument: " + Parameter.Name);
  }
  return Value;
}

template <class TMethod, size_t N, size_t... TIndices>
auto ReadArguments(
    const nlohmann::json& Arguments,
    const std::array<FAutomationParameterMetadata, N>& Parameters,
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
  static_assert(
      AutomationJsonWritable<TReturn>, "Automation return type must support JSON output."
  );
  return TAutomationJsonConverter<std::remove_cvref_t<TReturn>>::ToJson(
      std::forward<TReturn>(Result)
  );
}

template <class TMethod, size_t N, size_t... TIndices>
nlohmann::json MakeInputSchema(
    const std::array<FAutomationParameterMetadata, N>& Parameters, std::index_sequence<TIndices...>
) {
  using TTraits = TMethodTraits<TMethod>;
  nlohmann::json Properties = nlohmann::json::object();
  nlohmann::json Required = nlohmann::json::array();
  (
      [&] {
        using TArgument =
            TArgumentStorage<std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>;
        static_assert(
            AutomationJsonReadable<TArgument>, "Automation argument type must support JSON input."
        );
        const auto& Parameter = Parameters[TIndices];
        Properties[Parameter.Name] = TAutomationJsonConverter<TArgument>::GetSchema();
        if (!Parameter.Description.empty()) {
          Properties[Parameter.Name]["description"] = Parameter.Description;
        }
        if (Parameter.bRequired) {
          Required.push_back(Parameter.Name);
        }
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

template <class TOwner, class TMethod, size_t N, class TResultAdapter>
nlohmann::json InvokeMethod(
    TOwner& Owner,
    TMethod Method,
    const nlohmann::json& Arguments,
    const std::array<FAutomationParameterMetadata, N>& Parameters,
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

template <class TMethod, size_t N = 0, class TResultAdapter = std::nullptr_t>
void RegisterMethod(
    FAutomationRegistrationContext& Context,
    std::string Name,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    std::array<FAutomationParameterMetadata, N> Parameters = {},
    TResultAdapter ResultAdapter = nullptr
) {
  static_assert(
      std::is_member_function_pointer_v<TMethod>,
      "Automation methods must be non-static member functions."
  );
  using TTraits = TMethodTraits<TMethod>;
  using TOwner = typename TTraits::OwnerType;
  static_assert(
      std::derived_from<TOwner, AActor> || std::derived_from<TOwner, MActorComponent>,
      "Automation methods must belong to an AActor or MActorComponent."
  );
  static_assert(
      N == TTraits::ArgumentCount,
      "Automation parameter count does not match method argument count."
  );
  static_assert(
      std::same_as<std::remove_cvref_t<TResultAdapter>, std::nullptr_t> ||
          !std::is_void_v<typename TTraits::ReturnType>,
      "Void automation methods cannot use a result adapter."
  );

  nlohmann::json InputSchema =
      MakeInputSchema<TMethod>(Parameters, std::make_index_sequence<TTraits::ArgumentCount>{});

  if constexpr (std::is_base_of_v<AActor, TOwner>) {
    FAutomationActorHandler Handler =
        [Method, Parameters = std::move(Parameters), ResultAdapter = std::move(ResultAdapter)](
            AActor& Actor, const nlohmann::json& Arguments
        ) mutable {
          auto* TypedActor = dynamic_cast<TOwner*>(&Actor);
          if (!TypedActor) {
            throw std::runtime_error("Automation actor type mismatch.");
          }
          return InvokeMethod(*TypedActor, Method, Arguments, Parameters, ResultAdapter);
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
          return InvokeMethod(*TypedComponent, Method, Arguments, Parameters, ResultAdapter);
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

}  // namespace BroccoliAutomationDetail
