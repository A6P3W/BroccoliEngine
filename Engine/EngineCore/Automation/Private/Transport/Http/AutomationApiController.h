#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

#include "Runtime/AutomationCommandQueue.h"
#include "AutomationComponentMethodRegistry.h"
#include "AutomationMethodRegistry.h"
#include "AutomationSystemCommandRegistry.h"
#include "AutomationTypes.h"
#include "Runtime/AutomationRuntimeTypes.h"
#include "Transport/Http/AutomationHttpTypes.h"
#include "World/AutomationDiscoveryTypes.h"
#include "World/AutomationWorldTypes.h"

class FAutomationApiController {
 public:
  FAutomationApiController(
      FAutomationCommandQueue& InCommandQueue,
      const FAutomationConfig& InConfig,
      FAutomationStateProvider InStateProvider,
      FAutomationActorListProvider InActorListProvider = {},
      FAutomationActorProvider InActorProvider = {},
      FAutomationActorComponentListProvider InActorComponentListProvider = {},
      FAutomationSpawnActorProvider InSpawnActorProvider = {},
      FAutomationDestroyActorProvider InDestroyActorProvider = {},
      FAutomationPatchActorTransformProvider InPatchActorTransformProvider = {},
      FAutomationMethodRegistry* InMethodRegistry = nullptr,
      FAutomationActorResolver InActorResolver = {},
      FAutomationComponentMethodRegistry* InComponentMethodRegistry = nullptr,
      FAutomationComponentResolver InComponentResolver = {},
      FAutomationSystemCommandRegistry* InSystemCommandRegistry = nullptr,
      FAutomationActorClassListProvider InActorClassListProvider = {},
      FAutomationLevelListProvider InLevelListProvider = {},
      FAutomationActorClassExistsProvider InActorClassExistsProvider = {}
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors(const FAutomationActorQueryText& Query = {});
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse GetWorldActorComponents(std::string_view ActorIdText);
  FAutomationHttpResponse GetActorClasses();
  FAutomationHttpResponse GetLevels();
  FAutomationHttpResponse GetActorClassMethods(std::string_view ClassName);
  FAutomationHttpResponse CreateWorldActor(const nlohmann::json& Body);
  FAutomationHttpResponse DeleteWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse PatchWorldActorTransform(
      std::string_view ActorIdText, const nlohmann::json& Body
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
  FAutomationHttpResponse GetSystemCommands();
  FAutomationHttpResponse ExecuteSystemCommand(
      std::string_view CommandName, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetRecentLogs(const FAutomationLogQueryText& Query);

 private:
  static bool TryParseActorId(std::string_view Text, FActorId& OutActorId);
  static bool TryParseComponentId(std::string_view Text, FComponentId& OutComponentId);
  static bool TryParseSpawnRequest(
      const nlohmann::json& Body, FAutomationSpawnActorRequest& OutRequest, std::string& OutError
  );
  static bool TryParseTransformPatch(
      const nlohmann::json& Body, FAutomationTransformPatch& OutPatch, std::string& OutError
  );
  static nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor);
  static nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot);
  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
  FAutomationStateProvider StateProvider;
  FAutomationActorListProvider ActorListProvider;
  FAutomationActorProvider ActorProvider;
  FAutomationActorComponentListProvider ActorComponentListProvider;
  FAutomationSpawnActorProvider SpawnActorProvider;
  FAutomationDestroyActorProvider DestroyActorProvider;
  FAutomationPatchActorTransformProvider PatchActorTransformProvider;
  FAutomationMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorResolver ActorResolver;
  FAutomationComponentMethodRegistry* ComponentMethodRegistry = nullptr;
  FAutomationComponentResolver ComponentResolver;
  FAutomationSystemCommandRegistry* SystemCommandRegistry = nullptr;
  FAutomationActorClassListProvider ActorClassListProvider;
  FAutomationLevelListProvider LevelListProvider;
  FAutomationActorClassExistsProvider ActorClassExistsProvider;
};
