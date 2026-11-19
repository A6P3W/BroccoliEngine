#pragma once

#include "ActorRegistry.h"
#include "ReflectionGenerator.h"

template <class T>
struct TActorAutoRegister {
  TActorAutoRegister(bool IsGameMode = false) {
    if (!ActorRegistry::GetInstance().RegisterOwned<T>("Static", IsGameMode)) return;
    if (!ReflectionGenerator::RegisterStaticClass<T>()) {
      ActorRegistry::GetInstance().UnregisterClass(T::StaticClassName());
    }
  }
};

#define REGISTER_ACTOR(ClassName) static TActorAutoRegister<ClassName> AutoRegister_##ClassName;
