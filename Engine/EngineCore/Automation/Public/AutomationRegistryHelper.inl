#pragma once
#include <concepts>

#include "AutomationAutoRegistrar.h"

struct FAutomationParameterDefinition {
  std::string Name;
  std::string Description;
};
class FAutomationParameterList {
 public:
  FAutomationParameterList() = default;
  FAutomationParameterList(std::initializer_list<FAutomationParameterDefinition> Values)
      : Parameters(Values) {}
  const auto& GetParameters() const { return Parameters; }

 private:
  std::vector<FAutomationParameterDefinition> Parameters;
};
namespace AutomationHelper {
namespace Detail {
template <class R, class O, class... A>
struct TAutomationMethodTraits<R (O::*)(A...) noexcept> {
  using ActorType = O;
  using ReturnType = R;
  using ArgumentTuple = std::tuple<A...>;
  static constexpr size_t ArgumentCount = sizeof...(A);
};
template <class R, class O, class... A>
struct TAutomationMethodTraits<R (O::*)(A...) const noexcept> {
  using ActorType = O;
  using ReturnType = R;
  using ArgumentTuple = std::tuple<A...>;
  static constexpr size_t ArgumentCount = sizeof...(A);
};
template <class M, size_t... I>
auto ReadUnified(
    const nlohmann::json& Args, const FAutomationParameterList& Params, std::index_sequence<I...>
) {
  using T = TAutomationMethodTraits<M>;
  const auto& P = Params.GetParameters();
  return std::tuple<
      TAutomationArgumentStorage<std::tuple_element_t<I, typename T::ArgumentTuple>>...>{
      ReadArgument<TAutomationArgumentStorage<std::tuple_element_t<I, typename T::ArgumentTuple>>>(
          Args, {P[I].Name, P[I].Description, true}
      )...
  };
}
template <class M, class O, class Adapter>
nlohmann::json InvokeUnified(
    O& Object,
    M Method,
    const nlohmann::json& Args,
    const FAutomationParameterList& Params,
    Adapter& ResultAdapter
) {
  using T = TAutomationMethodTraits<M>;
  using R = typename T::ReturnType;
  auto Values = ReadUnified<M>(Args, Params, std::make_index_sequence<T::ArgumentCount>{});
  if constexpr (std::is_void_v<R>) {
    std::apply(
        [&](auto&&... V) { std::invoke(Method, Object, std::forward<decltype(V)>(V)...); },
        std::move(Values)
    );
    return nlohmann::json::object();
  } else {
    decltype(auto) Result = std::apply(
        [&](auto&&... V) -> decltype(auto) {
          return std::invoke(Method, Object, std::forward<decltype(V)>(V)...);
        },
        std::move(Values)
    );
    if constexpr (std::is_same_v<std::remove_cvref_t<Adapter>, std::nullptr_t>)
      return ConvertReturnValue(std::forward<decltype(Result)>(Result));
    else
      return nlohmann::json(std::invoke(ResultAdapter, std::forward<decltype(Result)>(Result)));
  }
}
template <class M, class Adapter>
void RegisterUnified(
    FAutomationMethodRegistry& AR,
    FAutomationComponentMethodRegistry& CR,
    std::string Name,
    std::string Description,
    EAutomationPermission Permission,
    M Method,
    FAutomationParameterList Params,
    Adapter AdapterValue
) {
  using T = TAutomationMethodTraits<M>;
  using O = typename T::ActorType;
  using R = typename T::ReturnType;
  static_assert(
      std::is_base_of_v<AActor, O> || std::is_base_of_v<MActorComponent, O>,
      "Automation methods must belong to an AActor or MActorComponent."
  );
  static_assert(
      std::is_same_v<std::remove_cvref_t<Adapter>, std::nullptr_t> || !std::is_void_v<R>,
      "Void automation methods cannot use a result adapter."
  );
  static_assert(
      std::is_same_v<std::remove_cvref_t<Adapter>, std::nullptr_t> ||
          (std::copy_constructible<std::remove_cvref_t<Adapter>> &&
           std::invocable<Adapter&, const R&>),
      "Automation result adapters must be copyable and accept the method return value."
  );
  if (Params.GetParameters().size() != T::ArgumentCount)
    throw std::runtime_error("Automation parameter count does not match method argument count.");
  nlohmann::json Properties = nlohmann::json::object();
  for (const auto& P : Params.GetParameters()) {
    Properties[P.Name] = {{"type", "string"}};
    if (!P.Description.empty()) Properties[P.Name]["description"] = P.Description;
  }
  nlohmann::json Schema = {
      {"type", "object"}, {"properties", Properties}, {"additionalProperties", false}
  };
  if constexpr (std::is_base_of_v<AActor, O>) {
    FAutomationMethodDescriptor D;
    D.Name = std::move(Name);
    D.Description = std::move(Description);
    D.Permission = Permission;
    D.InputSchema = std::move(Schema);
    D.Handler = [Method, Params = std::move(Params), AdapterValue = std::move(AdapterValue)](
                    AActor& A, const nlohmann::json& J
                ) mutable {
      auto* V = dynamic_cast<O*>(&A);
      if (!V) throw std::runtime_error("Automation actor type mismatch.");
      return InvokeUnified(*V, Method, J, Params, AdapterValue);
    };
    RegisterDescriptor(AR, O::StaticClassName(), std::move(D));
  } else {
    FAutomationComponentMethodDescriptor D;
    D.Name = std::move(Name);
    D.Description = std::move(Description);
    D.Permission = Permission;
    D.InputSchema = std::move(Schema);
    D.Handler = [Method, Params = std::move(Params), AdapterValue = std::move(AdapterValue)](
                    MActorComponent& C, const nlohmann::json& J
                ) mutable {
      auto* V = dynamic_cast<O*>(&C);
      if (!V) throw std::runtime_error("Automation component type mismatch.");
      return InvokeUnified(*V, Method, J, Params, AdapterValue);
    };
    RegisterComponentDescriptor(CR, O::StaticComponentClassName(), std::move(D));
  }
}
}  // namespace Detail
inline FAutomationParameterDefinition MakeAutomationParameter(std::string Name) {
  return {std::move(Name), {}};
}
inline FAutomationParameterDefinition MakeAutomationParameter(
    std::string Name, std::string Description
) {
  return {std::move(Name), std::move(Description)};
}
template <class M>
void RegisterMethod(
    FAutomationMethodRegistry& AR,
    FAutomationComponentMethodRegistry& CR,
    std::string N,
    std::string D,
    EAutomationPermission P,
    M Method
) {
  Detail::RegisterUnified(
      AR, CR, std::move(N), std::move(D), P, Method, FAutomationParameterList{}, nullptr
  );
}
template <class M, class X>
void RegisterMethod(
    FAutomationMethodRegistry& AR,
    FAutomationComponentMethodRegistry& CR,
    std::string N,
    std::string D,
    EAutomationPermission P,
    M Method,
    X Value
) {
  if constexpr (std::is_same_v<std::remove_cvref_t<X>, FAutomationParameterList>)
    Detail::RegisterUnified(
        AR, CR, std::move(N), std::move(D), P, Method, std::move(Value), nullptr
    );
  else
    Detail::RegisterUnified(
        AR, CR, std::move(N), std::move(D), P, Method, FAutomationParameterList{}, std::move(Value)
    );
}
template <class M, class X>
void RegisterMethod(
    FAutomationMethodRegistry& AR,
    FAutomationComponentMethodRegistry& CR,
    std::string N,
    std::string D,
    EAutomationPermission P,
    M Method,
    FAutomationParameterList Params,
    X Adapter
) {
  Detail::RegisterUnified(
      AR, CR, std::move(N), std::move(D), P, Method, std::move(Params), std::move(Adapter)
  );
}
}  // namespace AutomationHelper
#define AUTOMATION_PARAM(...) AutomationHelper::MakeAutomationParameter(__VA_ARGS__)
#define AUTOMATION_PARAMS(...) \
  FAutomationParameterList { __VA_ARGS__ }
#define REGISTER_AUTOMATION_METHOD(...)  \
  REGISTER_AUTOMATION_METHOD_INVOKE(     \
      REGISTER_AUTOMATION_METHOD_SELECT( \
          __VA_ARGS__,                   \
          REGISTER_AUTOMATION_METHOD_6,  \
          REGISTER_AUTOMATION_METHOD_5,  \
          REGISTER_AUTOMATION_METHOD_4   \
      ),                                 \
      __VA_ARGS__                        \
  )
#define REGISTER_AUTOMATION_METHOD_INVOKE(Macro, ...) Macro(__VA_ARGS__)
#define REGISTER_AUTOMATION_METHOD_SELECT(_1, _2, _3, _4, _5, _6, Name, ...) Name
#define REGISTER_AUTOMATION_METHOD_4(Name, Description, Permission, MemberFunction) \
  REGISTER_AUTOMATION_METHOD_REGISTER_EXPANDED(                                     \
      __COUNTER__, Name, Description, Permission, MemberFunction                    \
  )
#define REGISTER_AUTOMATION_METHOD_5(Name, Description, Permission, MemberFunction, Argument) \
  REGISTER_AUTOMATION_METHOD_REGISTER_EXPANDED(                                               \
      __COUNTER__, Name, Description, Permission, MemberFunction, Argument                    \
  )
#define REGISTER_AUTOMATION_METHOD_6(                                                \
    Name, Description, Permission, MemberFunction, Arguments, Adapter                \
)                                                                                    \
  REGISTER_AUTOMATION_METHOD_REGISTER_EXPANDED(                                      \
      __COUNTER__, Name, Description, Permission, MemberFunction, Arguments, Adapter \
  )
#define REGISTER_AUTOMATION_METHOD_REGISTER_EXPANDED(                     \
    Counter, Name, Description, Permission, MemberFunction, ...           \
)                                                                         \
  REGISTER_AUTOMATION_METHOD_REGISTER(                                    \
      Counter, Name, Description, Permission, MemberFunction, __VA_ARGS__ \
  )
#define REGISTER_AUTOMATION_METHOD_REGISTER(                    \
    Counter, Name, Description, Permission, MemberFunction, ... \
)                                                               \
  namespace {                                                   \
  void BROCCOLI_JOIN(GAutomationUnifiedFunction_, Counter)(     \
      FAutomationMethodRegistry & ActorRegistry,                \
      FAutomationComponentMethodRegistry& ComponentRegistry     \
  ) {                                                           \
    AutomationHelper::RegisterMethod(                           \
        ActorRegistry,                                          \
        ComponentRegistry,                                      \
        Name,                                                   \
        Description,                                            \
        Permission,                                             \
        MemberFunction,                                         \
        __VA_ARGS__                                             \
    );                                                          \
  }                                                             \
  const FAutomationUnifiedMethodAutoRegister                    \
      BROCCOLI_JOIN(GAutomationUnifiedRegistration_, Counter)(  \
          &BROCCOLI_JOIN(GAutomationUnifiedFunction_, Counter)  \
      );                                                        \
  }