#include "DiscoveryController.h"
#include "../Detail/HttpErrorMapping.h"
#include "../Detail/HttpParsing.h"
#include "../Detail/HttpSerialization.h"

using namespace AutomationHttpDetail;

FAutomationDiscoveryController::FAutomationDiscoveryController(
    FAutomationHttpRequestExecutor& InExecutor,
    FAutomationCommandQueue& InCommandQueue,
    FAutomationActorMethodRegistry& InMethodRegistry,
    FAutomationActorClassListProvider InActorClassListProvider,
    FAutomationLevelListProvider InLevelListProvider,
    FAutomationActorClassExistsProvider InActorClassExistsProvider
)
    : FAutomationHttpControllerBase(InExecutor),
      CommandQueue(InCommandQueue),
      MethodRegistry(&InMethodRegistry),
      ActorClassListProvider(std::move(InActorClassListProvider)),
      LevelListProvider(std::move(InLevelListProvider)),
      ActorClassExistsProvider(std::move(InActorClassExistsProvider)) {}

FAutomationHttpResponse FAutomationDiscoveryController::GetActorClasses() {
  try {
    if (!ActorClassListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = ActorClassListProvider]() {
      nlohmann::json Classes = nlohmann::json::array();
      for (const FAutomationActorClassInfo& Info : Provider()) {
        Classes.push_back({{"className", Info.ClassName}, {"isGameMode", Info.bIsGameMode}});
      }
      return MakeAutomationSuccess({{"classes", std::move(Classes)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationDiscoveryController::GetLevels() {
  try {
    if (!LevelListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = LevelListProvider]() {
      nlohmann::json Levels = nlohmann::json::array();
      for (const FAutomationLevelInfo& Info : Provider()) {
        Levels.push_back(
            {{"sceneId", Info.SceneId},
             {"levelName", std::filesystem::path(Info.LevelPath).stem().string()},
             {"levelPath", Info.LevelPath}}
        );
      }
      return MakeAutomationSuccess({{"levels", std::move(Levels)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationDiscoveryController::GetActorClassMethods(
    std::string_view ClassName
) {
  if (ClassName.empty() || ClassName.size() > 128) {
    return {
        400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, "The className is invalid.")
    };
  }
  try {
    if (!MethodRegistry || !ActorClassExistsProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Registry = MethodRegistry,
                              ExistsProvider = ActorClassExistsProvider,
                              ClassNameText = std::string(ClassName)]() {
          if (!ExistsProvider(ClassNameText)) {
            return MakeAutomationError(
                EAutomationErrorCode::ClassNotRegistered, ClassNotRegisteredMessage
            );
          }
          nlohmann::json Methods = nlohmann::json::array();
          for (const FAutomationMethodSnapshot& Snapshot :
               Registry->GetMethodsForClass(ClassNameText)) {
            if (!IsActorMethodPermissionAllowed(Snapshot.Permission)) {
              continue;
            }
            Methods.push_back(
                {{"name", Snapshot.Name},
                 {"description", Snapshot.Description},
                 {"inputSchema", Snapshot.InputSchema},
                 {"permission", ToAutomationPermissionString(Snapshot.Permission)}}
            );
          }
          return MakeAutomationSuccess(
              {{"className", ClassNameText}, {"methods", std::move(Methods)}}
          );
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

