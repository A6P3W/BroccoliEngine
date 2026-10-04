#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "BroccoliEngineAPI.h"

class AActor;
class MActorComponent;

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
  using FactoryFn = std::function<std::unique_ptr<MActorComponent>()>;
  static ComponentRegistry& GetInstance();

  template <class T>
  bool RegisterOwned(std::string ModuleOwner, FComponentClassOptions Options = {}) {
    static_assert(std::is_base_of_v<MActorComponent, T> && !std::is_abstract_v<T>);
    static_assert(std::is_default_constructible_v<T>);
    return RegisterFactory(
        T::StaticComponentClassName(),
        []() -> std::unique_ptr<MActorComponent> { return std::make_unique<T>(); },
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
