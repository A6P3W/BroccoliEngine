#pragma once

#include "ComponentRegistry.h"

namespace ReflectionGenerator {
template <class T>
bool RegisterStaticComponentClass();
}

#include "ReflectionGenerator.h"

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
