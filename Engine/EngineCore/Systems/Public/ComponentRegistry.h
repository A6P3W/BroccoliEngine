#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "Actor.h"
#include "BroccoliEngineAPI.h"

struct FComponentClassOptions {
  bool EditorAddable = true;
  bool AllowMultiple = true;
};

struct FComponentClassInfo {
  std::string ClassName;
  std::string ModuleOwner;
  FComponentClassOptions Options;
};

class BROCCOLI_ENGINE_API ComponentRegistry {
 public:
  using FactoryFn = std::function<MActorComponent*(AActor*, std::string_view)>;
  static ComponentRegistry& GetInstance();

  template <class T>
  bool RegisterOwned(std::string ModuleOwner, FComponentClassOptions Options = {}) {
    static_assert(std::is_base_of_v<MActorComponent, T> && !std::is_abstract_v<T>);
    static_assert(std::is_default_constructible_v<T>);
    return RegisterFactory(
        T::StaticComponentClassName(),
        [](AActor* Owner, std::string_view Name) -> MActorComponent* {
          return NewObjectWithParams<T>(
              Owner,
              FComponentCreateParams{
                  .Name = std::string(Name), .Source = EComponentCreationSource::Instance
              }
          );
        },
        std::move(ModuleOwner),
        Options
    );
  }

  MActorComponent* Create(
      AActor* Owner, std::string_view ClassName, std::string_view RequestedName = {}
  );
  bool Contains(std::string_view ClassName) const;
  std::vector<FComponentClassInfo> GetClasses() const;
  bool HasLiveComponents(std::string_view ModuleOwner) const;
  void NotifyCreated(const MActorComponent* Component, std::string_view ClassName);
  void NotifyDestroyed(const MActorComponent* Component);
  void UnregisterModule(std::string_view ModuleOwner);
  void UnregisterClass(std::string_view ClassName);

 private:
  ComponentRegistry();
  ~ComponentRegistry();
  bool RegisterFactory(
      std::string ClassName,
      FactoryFn Factory,
      std::string ModuleOwner,
      FComponentClassOptions Options
  );
  struct Impl;
  Impl* ImplPtr;
};

template <class T>
struct TComponentAutoRegister {
  explicit TComponentAutoRegister(FComponentClassOptions Options = {}) {
    if (!ComponentRegistry::GetInstance().RegisterOwned<T>("Static", Options)) return;
    if (!ReflectionGenerator::RegisterStaticComponentClass<T>())
      ComponentRegistry::GetInstance().UnregisterClass(T::StaticComponentClassName());
  }
};

#define REGISTER_COMPONENT(ClassName, ...) \
  static TComponentAutoRegister<ClassName> AutoRegisterComponent_##ClassName({__VA_ARGS__});
