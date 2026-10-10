#pragma once

#include <array>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ActorRegistry.h"
#include "ComponentRegistry.h"
#include "AutomationAnnotations.h"
#include "Detail/AutomationJsonConverter.h"
#include "Detail/AutomationRegistrationContext.h"
#include "FunctionReflection.h"

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
  static_assert(AutomationJsonReadable<TValue>, "Control argument type must support JSON input.");
  TValue Value{};
  const auto Iterator = Arguments.find(Parameter.Name);
  if (Iterator == Arguments.end()) {
    if constexpr (TIsOptional<TValue>::value) {
      return std::nullopt;
    }
    throw std::runtime_error("Missing control argument: " + Parameter.Name);
  }
  if (!TAutomationJsonConverter<TValue>::FromJson(*Iterator, Value)) {
    throw std::runtime_error("Invalid control argument: " + Parameter.Name);
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
            AutomationJsonReadable<TArgument>, "Control argument type must support JSON input."
        );
        const auto& Parameter = Parameters[TIndices];
        Properties[Parameter.Name] = TAutomationJsonConverter<TArgument>::GetSchema();
        if (!Parameter.Description.empty()) {
          Properties[Parameter.Name]["description"] = Parameter.Description;
        }
        if (!TIsOptional<TArgument>::value) {
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
    TMethod Method,
    std::array<FAutomationParameterMetadata, N> Parameters = {},
    TResultAdapter ResultAdapter = nullptr,
    std::string RegisteredClassName = {}
) {
  static_assert(
      std::is_member_function_pointer_v<TMethod>,
      "Control methods must be non-static member functions."
  );
  using TTraits = TMethodTraits<TMethod>;
  using TOwner = typename TTraits::OwnerType;
  static_assert(
      std::derived_from<TOwner, AActor> || std::derived_from<TOwner, MActorComponent>,
      "Control methods must belong to an AActor or MActorComponent."
  );
  static_assert(
      N == TTraits::ArgumentCount, "Control parameter count does not match method argument count."
  );
  static_assert(
      std::same_as<std::remove_cvref_t<TResultAdapter>, std::nullptr_t> ||
          !std::is_void_v<typename TTraits::ReturnType>,
      "Void control methods cannot use a result adapter."
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
            throw std::runtime_error("Control actor type mismatch.");
          }
          return InvokeMethod(*TypedActor, Method, Arguments, Parameters, ResultAdapter);
        };
    Context.RegisterActorMethod(
        RegisteredClassName.empty() ? TOwner::StaticClassName() : std::move(RegisteredClassName),
        std::move(Name),
        std::move(Description),
        std::move(InputSchema),
        std::move(Handler)
    );
  } else {
    FAutomationComponentHandler Handler =
        [Method, Parameters = std::move(Parameters), ResultAdapter = std::move(ResultAdapter)](
            MActorComponent& Component, const nlohmann::json& Arguments
        ) mutable {
          auto* TypedComponent = dynamic_cast<TOwner*>(&Component);
          if (!TypedComponent) {
            throw std::runtime_error("Control component type mismatch.");
          }
          return InvokeMethod(*TypedComponent, Method, Arguments, Parameters, ResultAdapter);
        };
    Context.RegisterComponentMethod(
        RegisteredClassName.empty() ? TOwner::StaticComponentClassName()
                                    : std::move(RegisteredClassName),
        std::move(Name),
        std::move(Description),
        std::move(InputSchema),
        std::move(Handler)
    );
  }
}

consteval bool IsValidControlName(std::string_view Name) {
  if (Name.empty() || Name.size() > 128 || Name[0] < 'a' || Name[0] > 'z') return false;
  for (const char Character : Name) {
    if ((Character < 'a' || Character > 'z') && (Character < '0' || Character > '9') &&
        Character != '_') {
      return false;
    }
  }
  return true;
}

template <std::meta::info Function>
consteval auto GetReflectedParameters() {
  constexpr std::size_t Count = std::meta::parameters_of(Function).size();
  std::array<FControlParameterAnnotation, Count> Result{};
  auto Reflected = std::meta::parameters_of(Function);
  for (std::size_t Index = 0; Index < Count; ++Index) {
    if (!std::meta::has_identifier(Reflected[Index])) continue;
    const std::string_view Identifier = std::meta::identifier_of(Reflected[Index]);
    if (Identifier.size() > Result[Index].Name.Data.size()) {
      throw "Control parameter name exceeds 128 bytes.";
    }
    Result[Index].Name.Size = Identifier.size();
    for (std::size_t Character = 0; Character < Identifier.size(); ++Character) {
      Result[Index].Name.Data[Character] = Identifier[Character];
    }
  }
  auto Annotations = std::meta::annotations_of_with_type(Function, ^^FControlParameterAnnotation);
  std::array<bool, Count> Seen{};
  for (auto Metadata : Annotations) {
    const auto Parameter = std::meta::extract<FControlParameterAnnotation>(Metadata);
    if (Parameter.Index >= Count) throw "Control parameter index is out of range.";
    if (Seen[Parameter.Index]) throw "Control parameter index is duplicated.";
    Seen[Parameter.Index] = true;
    Result[Parameter.Index] = Parameter;
  }
  for (const auto& Parameter : Result) {
    if (!IsValidControlName(Parameter.Name.View())) {
      throw "Control parameter name is invalid or unnamed.";
    }
  }
  return Result;
}

template <class T, std::meta::info Function>
void RegisterReflectedMethod(FAutomationRegistrationContext& Context) {
  constexpr auto Annotation = [] consteval {
    auto Annotations = std::meta::annotations_of_with_type(Function, ^^FControlMethodAnnotation);
    if (Annotations.size() != 1) throw "Control method requires one method annotation.";
    return std::meta::extract<FControlMethodAnnotation>(Annotations[0]);
  }();
  static_assert(IsValidControlName(Annotation.Name.View()), "Control method name is invalid.");
  static_assert(!Annotation.Description.View().empty(), "Control method description is empty.");
  constexpr auto NameText = Annotation.Name;
  constexpr auto DescriptionText = Annotation.Description;
  constexpr auto ReflectedParameters = GetReflectedParameters<Function>();
  std::array<FAutomationParameterMetadata, ReflectedParameters.size()> Parameters{};
  for (std::size_t Index = 0; Index < Parameters.size(); ++Index) {
    Parameters[Index] = {
        std::string(ReflectedParameters[Index].Name.View()),
        std::string(ReflectedParameters[Index].Description.View())
    };
  }
  std::string ClassName;
  if constexpr (std::derived_from<T, AActor>) {
    ClassName = T::StaticClassName();
  } else {
    ClassName = T::StaticComponentClassName();
  }
  if constexpr (Annotation.ResultAdapter == std::meta::info{}) {
    RegisterMethod(
        Context,
        std::string(NameText.View()),
        std::string(DescriptionText.View()),
        &[:Function:], std::move(Parameters), nullptr, std::move(ClassName)
    );
  } else {
    using TMethod = decltype(&[:Function:]);
    static_assert(!std::is_void_v<typename TMethodTraits<TMethod>::ReturnType>);
    RegisterMethod(
        Context,
        std::string(NameText.View()),
        std::string(DescriptionText.View()),
        &[:Function:], std::move(Parameters), &
               [:Annotation.ResultAdapter:], std::move(ClassName)
    );
  }
}

template <class T>
void RegisterClass(FAutomationRegistrationContext& Context) {
  if constexpr (std::derived_from<T, AActor>) {
    if (!ActorRegistry::GetInstance().Contains(T::StaticClassName())) {
      throw std::runtime_error("Control actor class must be registered first.");
    }
  } else if (!ComponentRegistry::GetInstance().Contains(T::StaticComponentClassName())) {
    throw std::runtime_error("Control component class must be registered first.");
  }
  auto Visitor = [&Context]<std::meta::info Function>() {
    RegisterReflectedMethod<T, Function>(Context);
  };
  FunctionReflection::ForEachAnnotatedFunction<T, FControlMethodAnnotation>(Visitor);
}

}  // namespace BroccoliAutomationDetail
