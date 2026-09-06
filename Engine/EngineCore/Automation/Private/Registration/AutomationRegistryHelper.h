#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "Actor.h"
#include "ActorComponent.h"
#include "AutomationComponentMethodRegistry.h"
#include "AutomationMacros.h"
#include "AutomationMethodRegistry.h"
#include "AutomationSystemCommandRegistry.h"
#include "Log.h"

template <class T>
using TAutomationArgumentStorage = std::remove_cvref_t<T>;

template <class T>
struct TAutomationMethodTraits;

template <class T, class = void>
struct TAutomationSystemCommandTraits;

template <class TReturn, class... TArgs>
struct TAutomationSystemCommandTraits<TReturn (*)(TArgs...)> {
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
};

template <class TReturn, class... TArgs>
struct TAutomationSystemCommandTraits<TReturn(TArgs...)> {
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
};

template <class TReturn, class TObject, class... TArgs>
struct TAutomationSystemCommandTraits<TReturn (TObject::*)(TArgs...)> {
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
};

template <class TReturn, class TObject, class... TArgs>
struct TAutomationSystemCommandTraits<TReturn (TObject::*)(TArgs...) const> {
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
};

template <class T>
struct TAutomationSystemCommandTraits<T, std::void_t<decltype(&std::remove_cvref_t<T>::operator())>>
    : TAutomationSystemCommandTraits<decltype(&std::remove_cvref_t<T>::operator())> {};

template <class TReturn, class TActor, class... TArgs>
struct TAutomationMethodTraits<TReturn (TActor::*)(TArgs...)> {
  using ActorType = TActor;
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
  static constexpr bool bIsConst = false;
};

template <class TReturn, class TActor, class... TArgs>
struct TAutomationMethodTraits<TReturn (TActor::*)(TArgs...) const> {
  using ActorType = TActor;
  using ReturnType = TReturn;
  using ArgumentTuple = std::tuple<TArgs...>;
  static constexpr size_t ArgumentCount = sizeof...(TArgs);
  static constexpr bool bIsConst = true;
};

template <class T>
struct TAutomationParameter {
  using ValueType = T;

  std::string Name;
  std::string Description;
  bool bRequired = true;
};

template <class T>
TAutomationParameter<T> AutomationParam(std::string Name, std::string Description) {
  return {std::move(Name), std::move(Description), true};
}

template <class T>
TAutomationParameter<std::optional<T>> AutomationOptionalParam(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description), false};
}

namespace AutomationHelper {
namespace Detail {
template <class T>
struct TIsOptional : std::false_type {};

template <class T>
struct TIsOptional<std::optional<T>> : std::true_type {};

template <class TParameter, class TArgument>
constexpr bool bParameterMatchesArgument =
    std::is_same_v<typename TParameter::ValueType, TAutomationArgumentStorage<TArgument>>;

template <class TParameter>
nlohmann::json MakeParameterSchema(const TParameter& Parameter) {
  using TValue = typename TParameter::ValueType;
  nlohmann::json Schema = TAutomationJsonConverter<TValue>::GetSchema();
  if (!Parameter.Description.empty()) {
    Schema["description"] = Parameter.Description;
  }
  return Schema;
}

template <class... TParameters>
nlohmann::json MakeInputSchema(const TParameters&... Parameters) {
  nlohmann::json Properties = nlohmann::json::object();
  nlohmann::json Required = nlohmann::json::array();
  (
      [&] {
        Properties[Parameters.Name] = MakeParameterSchema(Parameters);
        if (Parameters.bRequired) {
          Required.push_back(Parameters.Name);
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

template <class TValue, class TParameter>
TValue ReadArgument(const nlohmann::json& Arguments, const TParameter& Parameter) {
  TValue Value{};
  const auto Iterator = Arguments.find(Parameter.Name);
  if (Iterator == Arguments.end()) {
    if constexpr (TIsOptional<TValue>::value) {
      return Value;
    }
    throw std::runtime_error("Missing automation argument: " + Parameter.Name);
  }
  if (!TAutomationJsonConverter<TValue>::FromJson(*Iterator, Value)) {
    throw std::runtime_error("Invalid automation argument: " + Parameter.Name);
  }
  return Value;
}

template <class TTraits, class TParametersTuple, size_t... TIndices>
auto ReadArguments(
    const nlohmann::json& Arguments,
    const TParametersTuple& Parameters,
    std::index_sequence<TIndices...>
) {
  return std::tuple<TAutomationArgumentStorage<
      std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>...>{
      ReadArgument<TAutomationArgumentStorage<
          std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>>>(
          Arguments, std::get<TIndices>(Parameters)
      )...
  };
}

template <class TMethod, class TParametersTuple, size_t... TIndices>
consteval bool ParametersMatch(std::index_sequence<TIndices...>) {
  using TTraits = TAutomationMethodTraits<TMethod>;
  return (
      bParameterMatchesArgument<
          std::tuple_element_t<TIndices, TParametersTuple>,
          std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>> &&
      ...
  );
}

template <class TReturn>
nlohmann::json ConvertReturnValue(TReturn&& Result) {
  return TAutomationJsonConverter<std::remove_cvref_t<TReturn>>::ToJson(
      std::forward<TReturn>(Result)
  );
}

template <class TObject, class TMethod, class TParametersTuple>
nlohmann::json InvokeRegisteredMethod(
    TObject& Object,
    TMethod Method,
    const nlohmann::json& Arguments,
    const TParametersTuple& Parameters
) {
  using TTraits = TAutomationMethodTraits<TMethod>;
  using TReturn = typename TTraits::ReturnType;

  auto Values = ReadArguments<TTraits>(
      Arguments, Parameters, std::make_index_sequence<TTraits::ArgumentCount>{}
  );
  if constexpr (std::is_void_v<TReturn>) {
    std::apply(
        [&Object, Method](auto&&... MethodArguments) {
          std::invoke(Method, Object, std::forward<decltype(MethodArguments)>(MethodArguments)...);
        },
        std::move(Values)
    );
    return nlohmann::json(nullptr);
  } else {
    decltype(auto) Result = std::apply(
        [&Object, Method](auto&&... MethodArguments) -> decltype(auto) {
          return std::invoke(
              Method, Object, std::forward<decltype(MethodArguments)>(MethodArguments)...
          );
        },
        std::move(Values)
    );
    return ConvertReturnValue(std::forward<decltype(Result)>(Result));
  }
}
inline void RegisterDescriptor(
    FAutomationMethodRegistry& Registry,
    std::string ClassName,
    FAutomationMethodDescriptor Descriptor
) {
  std::string Error;
  if (!Registry.RegisterMethod(std::move(ClassName), std::move(Descriptor), &Error)) {
    M_LOG(Error, "Automation method registration failed: {}", Error);
    throw std::runtime_error(Error);
  }
}

inline void RegisterComponentDescriptor(
    FAutomationComponentMethodRegistry& Registry,
    std::string ClassName,
    FAutomationComponentMethodDescriptor Descriptor
) {
  std::string Error;
  if (!Registry.RegisterMethod(std::move(ClassName), std::move(Descriptor), &Error)) {
    M_LOG(Error, "Automation component method registration failed: {}", Error);
    throw std::runtime_error(Error);
  }
}

inline void RegisterSystemCommandDescriptor(
    FAutomationSystemCommandRegistry& Registry, FAutomationSystemCommandDescriptor Descriptor
) {
  std::string Error;
  if (!Registry.RegisterCommand(std::move(Descriptor), &Error)) {
    M_LOG(Error, "Automation system command registration failed: {}", Error);
    throw std::runtime_error(Error);
  }
}

template <class TCallable, class TParametersTuple, size_t... TIndices>
consteval bool SystemCommandParametersMatch(std::index_sequence<TIndices...>) {
  using TTraits = TAutomationSystemCommandTraits<std::remove_cvref_t<TCallable>>;
  return (
      bParameterMatchesArgument<
          std::tuple_element_t<TIndices, TParametersTuple>,
          std::tuple_element_t<TIndices, typename TTraits::ArgumentTuple>> &&
      ...
  );
}
}  // namespace Detail

template <class TMethod, class... TParameters>
void RegisterMethod(
    FAutomationMethodRegistry& Registry,
    std::string MethodName,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    TParameters... Parameters
) {
  using TTraits = TAutomationMethodTraits<TMethod>;
  using TActor = typename TTraits::ActorType;
  using TReturn = typename TTraits::ReturnType;
  using TParametersTuple = std::tuple<TParameters...>;
  static_assert(std::is_base_of_v<AActor, TActor>, "Automation methods must belong to an Actor.");
  static_assert(
      TTraits::ArgumentCount == sizeof...(TParameters),
      "Every automation method argument requires one parameter definition."
  );
  static_assert(
      Detail::ParametersMatch<TMethod, TParametersTuple>(
          std::make_index_sequence<sizeof...(TParameters)>{}
      ),
      "Automation parameter types must match the method argument types."
  );

  TParametersTuple ParameterDefinitions(std::move(Parameters)...);
  FAutomationMethodDescriptor Descriptor;
  Descriptor.Name = std::move(MethodName);
  Descriptor.Description = std::move(Description);
  Descriptor.Permission = Permission;
  Descriptor.InputSchema = std::apply(
      [](const auto&... Values) { return Detail::MakeInputSchema(Values...); }, ParameterDefinitions
  );
  Descriptor.Handler = [Method, ParameterDefinitions = std::move(ParameterDefinitions)](
                           AActor& Actor, const nlohmann::json& Arguments
                       ) {
    auto* TypedActor = dynamic_cast<TActor*>(&Actor);
    if (!TypedActor) {
      throw std::runtime_error("Automation actor type mismatch.");
    }
    return Detail::InvokeRegisteredMethod(*TypedActor, Method, Arguments, ParameterDefinitions);
  };

  Detail::RegisterDescriptor(Registry, TActor::StaticClassName(), std::move(Descriptor));
}

template <class TMethod, class... TParameters>
void RegisterComponentMethod(
    FAutomationComponentMethodRegistry& Registry,
    std::string MethodName,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    TParameters... Parameters
) {
  using TTraits = TAutomationMethodTraits<TMethod>;
  using TComponent = typename TTraits::ActorType;
  using TParametersTuple = std::tuple<TParameters...>;
  static_assert(
      std::is_base_of_v<MActorComponent, TComponent>,
      "Automation component methods must belong to an MActorComponent."
  );
  static_assert(
      TTraits::ArgumentCount == sizeof...(TParameters),
      "Every automation component method argument requires one parameter definition."
  );
  static_assert(
      Detail::ParametersMatch<TMethod, TParametersTuple>(
          std::make_index_sequence<sizeof...(TParameters)>{}
      ),
      "Automation parameter types must match the method argument types."
  );

  TParametersTuple ParameterDefinitions(std::move(Parameters)...);
  FAutomationComponentMethodDescriptor Descriptor;
  Descriptor.Name = std::move(MethodName);
  Descriptor.Description = std::move(Description);
  Descriptor.Permission = Permission;
  Descriptor.InputSchema = std::apply(
      [](const auto&... Values) { return Detail::MakeInputSchema(Values...); }, ParameterDefinitions
  );
  Descriptor.Handler = [Method, ParameterDefinitions = std::move(ParameterDefinitions)](
                           MActorComponent& Component, const nlohmann::json& Arguments
                       ) {
    auto* TypedComponent = dynamic_cast<TComponent*>(&Component);
    if (!TypedComponent) {
      throw std::runtime_error("Automation component type mismatch.");
    }
    return Detail::InvokeRegisteredMethod(*TypedComponent, Method, Arguments, ParameterDefinitions);
  };

  Detail::RegisterComponentDescriptor(
      Registry, TComponent::StaticComponentClassName(), std::move(Descriptor)
  );
}

template <class TMethod, class TResultAdapter, class... TParameters>
void RegisterMethodWithResultAdapter(
    FAutomationMethodRegistry& Registry,
    std::string MethodName,
    std::string Description,
    EAutomationPermission Permission,
    TMethod Method,
    TResultAdapter ResultAdapter,
    TParameters... Parameters
) {
  using TTraits = TAutomationMethodTraits<TMethod>;
  using TActor = typename TTraits::ActorType;
  using TReturn = typename TTraits::ReturnType;
  using TParametersTuple = std::tuple<TParameters...>;
  static_assert(std::is_base_of_v<AActor, TActor>, "Automation methods must belong to an Actor.");
  static_assert(
      TTraits::ArgumentCount == sizeof...(TParameters),
      "Every automation method argument requires one parameter definition."
  );
  static_assert(
      Detail::ParametersMatch<TMethod, TParametersTuple>(
          std::make_index_sequence<sizeof...(TParameters)>{}
      ),
      "Automation parameter types must match the method argument types."
  );

  TParametersTuple ParameterDefinitions(std::move(Parameters)...);
  FAutomationMethodDescriptor Descriptor;
  Descriptor.Name = std::move(MethodName);
  Descriptor.Description = std::move(Description);
  Descriptor.Permission = Permission;
  Descriptor.InputSchema = std::apply(
      [](const auto&... Values) { return Detail::MakeInputSchema(Values...); }, ParameterDefinitions
  );
  Descriptor.Handler = [Method,
                        ResultAdapter = std::move(ResultAdapter),
                        ParameterDefinitions = std::move(ParameterDefinitions)](
                           AActor& Actor, const nlohmann::json& Arguments
                       ) {
    auto* TypedActor = dynamic_cast<TActor*>(&Actor);
    if (!TypedActor) {
      throw std::runtime_error("Automation actor type mismatch.");
    }
    auto Values = Detail::ReadArguments<TTraits>(
        Arguments, ParameterDefinitions, std::make_index_sequence<TTraits::ArgumentCount>{}
    );
    if constexpr (std::is_void_v<TReturn>) {
      std::apply(
          [TypedActor, Method](auto&&... MethodArguments) {
            std::invoke(
                Method, *TypedActor, std::forward<decltype(MethodArguments)>(MethodArguments)...
            );
          },
          std::move(Values)
      );
      return std::invoke(ResultAdapter, *TypedActor);
    } else {
      decltype(auto) Result = std::apply(
          [TypedActor, Method](auto&&... MethodArguments) -> decltype(auto) {
            return std::invoke(
                Method, *TypedActor, std::forward<decltype(MethodArguments)>(MethodArguments)...
            );
          },
          std::move(Values)
      );
      return std::invoke(ResultAdapter, *TypedActor, std::forward<decltype(Result)>(Result));
    }
  };

  Detail::RegisterDescriptor(Registry, TActor::StaticClassName(), std::move(Descriptor));
}

template <class TCallable, class... TParameters>
void RegisterSystemCommand(
    FAutomationSystemCommandRegistry& Registry,
    std::string CommandName,
    std::string Description,
    TCallable Callable,
    TParameters... Parameters
) {
  using TTraits = TAutomationSystemCommandTraits<std::remove_cvref_t<TCallable>>;
  using TReturn = typename TTraits::ReturnType;
  using TParametersTuple = std::tuple<TParameters...>;
  static_assert(
      TTraits::ArgumentCount == sizeof...(TParameters),
      "Every automation system command argument requires one parameter definition."
  );
  static_assert(
      Detail::SystemCommandParametersMatch<TCallable, TParametersTuple>(
          std::make_index_sequence<sizeof...(TParameters)>{}
      ),
      "Automation parameter types must match the system command argument types."
  );

  TParametersTuple ParameterDefinitions(std::move(Parameters)...);
  FAutomationSystemCommandDescriptor Descriptor;
  Descriptor.Name = std::move(CommandName);
  Descriptor.Description = std::move(Description);
  Descriptor.Permission = EAutomationPermission::SystemMutation;
  Descriptor.InputSchema = std::apply(
      [](const auto&... Values) { return Detail::MakeInputSchema(Values...); }, ParameterDefinitions
  );
  Descriptor.Handler = [Callable = std::move(Callable),
                        ParameterDefinitions = std::move(ParameterDefinitions)](
                           const nlohmann::json& Arguments
                       ) mutable {
    auto Values = Detail::ReadArguments<TTraits>(
        Arguments, ParameterDefinitions, std::make_index_sequence<TTraits::ArgumentCount>{}
    );
    if constexpr (std::is_void_v<TReturn>) {
      std::apply(
          [&Callable](auto&&... CommandArguments) {
            std::invoke(Callable, std::forward<decltype(CommandArguments)>(CommandArguments)...);
          },
          std::move(Values)
      );
      return nlohmann::json(nullptr);
    } else {
      decltype(auto) Result = std::apply(
          [&Callable](auto&&... CommandArguments) -> decltype(auto) {
            return std::invoke(
                Callable, std::forward<decltype(CommandArguments)>(CommandArguments)...
            );
          },
          std::move(Values)
      );
      return TAutomationJsonConverter<std::remove_cvref_t<TReturn>>::ToJson(Result);
    }
  };

  Detail::RegisterSystemCommandDescriptor(Registry, std::move(Descriptor));
}

template <class TCallable, class TResultAdapter, class... TParameters>
void RegisterSystemCommandWithResultAdapter(
    FAutomationSystemCommandRegistry& Registry,
    std::string CommandName,
    std::string Description,
    TCallable Callable,
    TResultAdapter ResultAdapter,
    TParameters... Parameters
) {
  using TTraits = TAutomationSystemCommandTraits<std::remove_cvref_t<TCallable>>;
  using TReturn = typename TTraits::ReturnType;
  using TParametersTuple = std::tuple<TParameters...>;
  static_assert(
      TTraits::ArgumentCount == sizeof...(TParameters),
      "Every automation system command argument requires one parameter definition."
  );
  static_assert(
      Detail::SystemCommandParametersMatch<TCallable, TParametersTuple>(
          std::make_index_sequence<sizeof...(TParameters)>{}
      ),
      "Automation parameter types must match the system command argument types."
  );

  TParametersTuple ParameterDefinitions(std::move(Parameters)...);
  FAutomationSystemCommandDescriptor Descriptor;
  Descriptor.Name = std::move(CommandName);
  Descriptor.Description = std::move(Description);
  Descriptor.Permission = EAutomationPermission::SystemMutation;
  Descriptor.InputSchema = std::apply(
      [](const auto&... Values) { return Detail::MakeInputSchema(Values...); }, ParameterDefinitions
  );
  Descriptor.Handler = [Callable = std::move(Callable),
                        ResultAdapter = std::move(ResultAdapter),
                        ParameterDefinitions = std::move(ParameterDefinitions)](
                           const nlohmann::json& Arguments
                       ) mutable {
    auto Values = Detail::ReadArguments<TTraits>(
        Arguments, ParameterDefinitions, std::make_index_sequence<TTraits::ArgumentCount>{}
    );
    if constexpr (std::is_void_v<TReturn>) {
      std::apply(
          [&Callable](auto&&... CommandArguments) {
            std::invoke(Callable, std::forward<decltype(CommandArguments)>(CommandArguments)...);
          },
          std::move(Values)
      );
      return std::invoke(ResultAdapter);
    } else {
      decltype(auto) Result = std::apply(
          [&Callable](auto&&... CommandArguments) -> decltype(auto) {
            return std::invoke(
                Callable, std::forward<decltype(CommandArguments)>(CommandArguments)...
            );
          },
          std::move(Values)
      );
      return std::invoke(ResultAdapter, std::forward<decltype(Result)>(Result));
    }
  };

  Detail::RegisterSystemCommandDescriptor(Registry, std::move(Descriptor));
}
}  // namespace AutomationHelper
