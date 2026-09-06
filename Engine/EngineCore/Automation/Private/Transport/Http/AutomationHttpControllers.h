#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

#include "AutomationComponentMethodRegistry.h"
#include "AutomationMethodRegistry.h"
#include "AutomationSystemCommandRegistry.h"
#include "AutomationTypes.h"
#include "Runtime/AutomationCommandQueue.h"
#include "Runtime/AutomationRuntimeTypes.h"
#include "Transport/Http/AutomationHttpTypes.h"
#include "World/AutomationDiscoveryTypes.h"
#include "World/AutomationWorldTypes.h"

class FAutomationHttpRequestExecutor {
 public:
  FAutomationHttpRequestExecutor(
      FAutomationCommandQueue& InCommandQueue, const FAutomationConfig& InConfig
  );

  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);

 private:
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
};

class FAutomationHttpControllerBase {
 protected:
  explicit FAutomationHttpControllerBase(FAutomationHttpRequestExecutor& InExecutor)
      : Executor(InExecutor) {}

  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket) {
    return Executor.WaitForResult(std::move(Ticket));
  }

  FAutomationHttpRequestExecutor& Executor;
};

class FAutomationWorldController final : public FAutomationHttpControllerBase {
 public:
  FAutomationWorldController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationStateProvider InStateProvider,
      FAutomationActorListProvider InActorListProvider,
      FAutomationActorProvider InActorProvider,
      FAutomationActorComponentListProvider InActorComponentListProvider,
      FAutomationSpawnActorProvider InSpawnActorProvider,
      FAutomationDestroyActorProvider InDestroyActorProvider,
      FAutomationPatchActorTransformProvider InPatchActorTransformProvider
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors(const FAutomationActorQueryText& Query = {});
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse GetWorldActorComponents(std::string_view ActorIdText);
  FAutomationHttpResponse CreateWorldActor(const nlohmann::json& Body);
  FAutomationHttpResponse DeleteWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse PatchWorldActorTransform(
      std::string_view ActorIdText, const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationStateProvider StateProvider;
  FAutomationActorListProvider ActorListProvider;
  FAutomationActorProvider ActorProvider;
  FAutomationActorComponentListProvider ActorComponentListProvider;
  FAutomationSpawnActorProvider SpawnActorProvider;
  FAutomationDestroyActorProvider DestroyActorProvider;
  FAutomationPatchActorTransformProvider PatchActorTransformProvider;
};

class FAutomationDiscoveryController final : public FAutomationHttpControllerBase {
 public:
  FAutomationDiscoveryController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationMethodRegistry& InMethodRegistry,
      FAutomationActorClassListProvider InActorClassListProvider,
      FAutomationLevelListProvider InLevelListProvider,
      FAutomationActorClassExistsProvider InActorClassExistsProvider
  );

  FAutomationHttpResponse GetActorClasses();
  FAutomationHttpResponse GetLevels();
  FAutomationHttpResponse GetActorClassMethods(std::string_view ClassName);

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorClassListProvider ActorClassListProvider;
  FAutomationLevelListProvider LevelListProvider;
  FAutomationActorClassExistsProvider ActorClassExistsProvider;
};

class FAutomationInvocationController final : public FAutomationHttpControllerBase {
 public:
  FAutomationInvocationController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationMethodRegistry& InMethodRegistry,
      FAutomationActorResolver InActorResolver,
      FAutomationComponentMethodRegistry& InComponentMethodRegistry,
      FAutomationComponentResolver InComponentResolver
  );

  FAutomationHttpResponse GetWorldActorMethods(std::string_view ActorIdText);
  FAutomationHttpResponse InvokeWorldActorMethod(
      std::string_view ActorIdText, std::string_view MethodName, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetWorldActorComponentMethods(
      std::string_view ActorIdText, std::string_view ComponentIdText
  );
  FAutomationHttpResponse InvokeWorldActorComponentMethod(
      std::string_view ActorIdText,
      std::string_view ComponentIdText,
      std::string_view MethodName,
      const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorResolver ActorResolver;
  FAutomationComponentMethodRegistry* ComponentMethodRegistry = nullptr;
  FAutomationComponentResolver ComponentResolver;
};

class FAutomationSystemController final : public FAutomationHttpControllerBase {
 public:
  FAutomationSystemController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationSystemCommandRegistry& InSystemCommandRegistry
  );

  FAutomationHttpResponse GetSystemCommands();
  FAutomationHttpResponse ExecuteSystemCommand(
      std::string_view CommandName, const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationSystemCommandRegistry* SystemCommandRegistry = nullptr;
};

class FAutomationLogController final : public FAutomationHttpControllerBase {
 public:
  FAutomationLogController(
      FAutomationHttpRequestExecutor& InExecutor, FAutomationCommandQueue& InCommandQueue
  );

  FAutomationHttpResponse GetRecentLogs(const FAutomationLogQueryText& Query);

 private:
  FAutomationCommandQueue& CommandQueue;
};

struct FAutomationHttpControllers {
  FAutomationWorldController& World;
  FAutomationDiscoveryController& Discovery;
  FAutomationInvocationController& Invocation;
  FAutomationSystemController& System;
  FAutomationLogController& Log;
};
