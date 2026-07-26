#include "AutomationHttpServer.h"

#include <httplib.h>

#include <atomic>
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "AutomationApiController.h"
#include "Log.h"

namespace {
constexpr std::string_view JsonContentType = "application/json; charset=utf-8";
constexpr std::string_view RequestTooLargeMessage =
    "The request body exceeds the maximum allowed size.";
constexpr std::string_view MethodNotAllowedMessage =
    "The HTTP method is not allowed for this endpoint.";
constexpr std::string_view RouteNotFoundMessage =
    "The requested automation endpoint was not found.";
constexpr std::string_view InternalErrorMessage =
    "The automation HTTP request failed unexpectedly.";
constexpr std::string_view InvalidContentTypeMessage = "The Content-Type must be application/json.";
constexpr std::string_view InvalidJsonMessage = "The request body is not valid JSON.";
constexpr std::string_view InvalidJsonRootMessage = "The JSON request root must be an object.";

void SetJsonResponse(httplib::Response& Response, int StatusCode, const nlohmann::json& Body) {
  Response.status = StatusCode;
  Response.set_header("Cache-Control", "no-store");
  Response.set_content(Body.dump(), std::string(JsonContentType));
}

bool TryParseJsonBody(
    const httplib::Request& Request,
    httplib::Response& Response,
    size_t MaxRequestBodyBytes,
    nlohmann::json& OutBody
) {
  if (Request.body.size() > MaxRequestBodyBytes) {
    SetJsonResponse(
        Response,
        413,
        MakeAutomationError(EAutomationErrorCode::RequestTooLarge, RequestTooLargeMessage)
    );
    return false;
  }

  std::string ContentType = Request.get_header_value("Content-Type");
  const size_t Separator = ContentType.find(';');
  if (Separator != std::string::npos) {
    ContentType.resize(Separator);
  }
  while (!ContentType.empty() && ContentType.back() == ' ') {
    ContentType.pop_back();
  }
  if (ContentType != "application/json") {
    SetJsonResponse(
        Response,
        400,
        MakeAutomationError(EAutomationErrorCode::InvalidRequest, InvalidContentTypeMessage)
    );
    return false;
  }

  if (Request.body.empty()) {
    SetJsonResponse(
        Response, 400, MakeAutomationError(EAutomationErrorCode::InvalidJson, InvalidJsonMessage)
    );
    return false;
  }

  OutBody = nlohmann::json::parse(Request.body, nullptr, false);
  if (OutBody.is_discarded()) {
    SetJsonResponse(
        Response, 400, MakeAutomationError(EAutomationErrorCode::InvalidJson, InvalidJsonMessage)
    );
    return false;
  }
  if (!OutBody.is_object()) {
    SetJsonResponse(
        Response,
        400,
        MakeAutomationError(EAutomationErrorCode::InvalidRequest, InvalidJsonRootMessage)
    );
    return false;
  }
  return true;
}

FAutomationLogQueryText ParseLogQuery(const httplib::Request& Request) {
  FAutomationLogQueryText Query;
  for (const auto& [Name, Value] : Request.params) {
    std::optional<std::string>* Target = nullptr;
    if (Name == "limit") {
      Target = &Query.Limit;
    } else if (Name == "level") {
      Target = &Query.Level;
    } else if (Name == "afterSequence") {
      Target = &Query.AfterSequence;
    } else {
      Query.bHasUnknownParameter = true;
      continue;
    }

    if (*Target) {
      Query.bHasDuplicateParameter = true;
    } else {
      *Target = Value;
    }
  }
  return Query;
}

FAutomationActorQueryText ParseActorQuery(const httplib::Request& Request) {
  FAutomationActorQueryText Query;
  for (const auto& [Name, Value] : Request.params) {
    std::optional<std::string>* Target = nullptr;
    if (Name == "className") {
      Target = &Query.ClassName;
    } else if (Name == "instanceName") {
      Target = &Query.InstanceName;
    } else {
      Query.bHasUnknownParameter = true;
      continue;
    }
    if (*Target) {
      Query.bHasDuplicateParameter = true;
    } else {
      *Target = Value;
    }
  }
  return Query;
}
}  // namespace

struct FAutomationHttpServer::Impl {
  Impl(const FAutomationConfig& InConfig, FAutomationApiController& InApiController)
      : Config(InConfig), ApiController(InApiController) {
    RegisterRoutes();
  }

  void RegisterRoutes() {
    Server.set_payload_max_length(Config.MaxRequestBodyBytes);

    Server.Get(
        "/api/v1/state", [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            if (Request.body.size() > Config.MaxRequestBodyBytes) {
              SetJsonResponse(
                  Response,
                  413,
                  MakeAutomationError(EAutomationErrorCode::RequestTooLarge, RequestTooLargeMessage)
              );
              return;
            }

            const FAutomationHttpResponse ApiResponse = ApiController.GetState();
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (const std::exception&) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        "/api/v1/world/actors",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const FAutomationHttpResponse ApiResponse =
                ApiController.GetWorldActors(ParseActorQuery(Request));
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        "/api/v1/logs/recent",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const FAutomationHttpResponse ApiResponse =
                ApiController.GetRecentLogs(ParseLogQuery(Request));
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        "/api/v1/system/commands", [this](const httplib::Request&, httplib::Response& Response) {
          try {
            const FAutomationHttpResponse ApiResponse = ApiController.GetSystemCommands();
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        "/api/v1/actor-classes", [this](const httplib::Request&, httplib::Response& Response) {
          try {
            const FAutomationHttpResponse ApiResponse = ApiController.GetActorClasses();
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get("/api/v1/levels", [this](const httplib::Request&, httplib::Response& Response) {
      try {
        const FAutomationHttpResponse ApiResponse = ApiController.GetLevels();
        SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
      } catch (...) {
        SetJsonResponse(
            Response,
            500,
            MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
        );
      }
    });
    Server.Get(
        R"(/api/v1/actor-classes/([^/]+)/methods)",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const std::string ClassName =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.GetActorClassMethods(ClassName);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        R"(/api/v1/world/actors/([0-9]+)/components)",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.GetWorldActorComponents(ActorIdText);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        R"(/api/v1/world/actors/([0-9]+)/methods)",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.GetWorldActorMethods(ActorIdText);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Get(
        R"(/api/v1/world/actors/([0-9]+))",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse = ApiController.GetWorldActor(ActorIdText);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Post(
        "/api/v1/world/actors",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            nlohmann::json Body;
            if (!TryParseJsonBody(Request, Response, Config.MaxRequestBodyBytes, Body)) {
              return;
            }
            const FAutomationHttpResponse ApiResponse = ApiController.CreateWorldActor(Body);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Delete(
        R"(/api/v1/world/actors/([0-9]+))",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse = ApiController.DeleteWorldActor(ActorIdText);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Patch(
        R"(/api/v1/world/actors/([0-9]+)/transform)",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            nlohmann::json Body;
            if (!TryParseJsonBody(Request, Response, Config.MaxRequestBodyBytes, Body)) {
              return;
            }
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.PatchWorldActorTransform(ActorIdText, Body);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Post(
        R"(/api/v1/world/actors/([0-9]+)/methods/([a-z][a-z0-9_]{0,127}))",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            nlohmann::json Body;
            if (!TryParseJsonBody(Request, Response, Config.MaxRequestBodyBytes, Body)) {
              return;
            }
            const std::string ActorIdText =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const std::string MethodName =
                Request.matches.size() > 2 ? Request.matches[2].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.InvokeWorldActorMethod(ActorIdText, MethodName, Body);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );
    Server.Post(
        R"(/api/v1/system/commands/([a-z][a-z0-9_]{0,127}))",
        [this](const httplib::Request& Request, httplib::Response& Response) {
          try {
            nlohmann::json Body;
            if (!TryParseJsonBody(Request, Response, Config.MaxRequestBodyBytes, Body)) {
              return;
            }
            const std::string CommandName =
                Request.matches.size() > 1 ? Request.matches[1].str() : std::string();
            const FAutomationHttpResponse ApiResponse =
                ApiController.ExecuteSystemCommand(CommandName, Body);
            SetJsonResponse(Response, ApiResponse.StatusCode, ApiResponse.Body);
          } catch (...) {
            SetJsonResponse(
                Response,
                500,
                MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
            );
          }
        }
    );

    const auto MethodNotAllowedHandler = [](const httplib::Request&, httplib::Response& Response) {
      SetJsonResponse(
          Response,
          405,
          MakeAutomationError(EAutomationErrorCode::InvalidRequest, MethodNotAllowedMessage)
      );
    };
    Server.Post("/api/v1/state", MethodNotAllowedHandler);
    Server.Put("/api/v1/state", MethodNotAllowedHandler);
    Server.Patch("/api/v1/state", MethodNotAllowedHandler);
    Server.Delete("/api/v1/state", MethodNotAllowedHandler);
    Server.Options("/api/v1/state", MethodNotAllowedHandler);
    Server.Post("/api/v1/logs/recent", MethodNotAllowedHandler);
    Server.Put("/api/v1/logs/recent", MethodNotAllowedHandler);
    Server.Patch("/api/v1/logs/recent", MethodNotAllowedHandler);
    Server.Delete("/api/v1/logs/recent", MethodNotAllowedHandler);
    Server.Options("/api/v1/logs/recent", MethodNotAllowedHandler);
    Server.Post("/api/v1/system/commands", MethodNotAllowedHandler);
    Server.Put("/api/v1/system/commands", MethodNotAllowedHandler);
    Server.Patch("/api/v1/system/commands", MethodNotAllowedHandler);
    Server.Delete("/api/v1/system/commands", MethodNotAllowedHandler);
    Server.Options("/api/v1/system/commands", MethodNotAllowedHandler);
    Server.Put("/api/v1/world/actors", MethodNotAllowedHandler);
    Server.Patch("/api/v1/world/actors", MethodNotAllowedHandler);
    Server.Delete("/api/v1/world/actors", MethodNotAllowedHandler);
    Server.Options("/api/v1/world/actors", MethodNotAllowedHandler);

    const std::string ActorRoute = R"(/api/v1/world/actors/([0-9]+))";
    Server.Post(ActorRoute, MethodNotAllowedHandler);
    Server.Put(ActorRoute, MethodNotAllowedHandler);
    Server.Patch(ActorRoute, MethodNotAllowedHandler);
    Server.Options(ActorRoute, MethodNotAllowedHandler);

    const std::string TransformRoute = R"(/api/v1/world/actors/([0-9]+)/transform)";
    Server.Get(TransformRoute, MethodNotAllowedHandler);
    Server.Post(TransformRoute, MethodNotAllowedHandler);
    Server.Put(TransformRoute, MethodNotAllowedHandler);
    Server.Delete(TransformRoute, MethodNotAllowedHandler);
    Server.Options(TransformRoute, MethodNotAllowedHandler);

    const std::string ActorMethodsRoute = R"(/api/v1/world/actors/([0-9]+)/methods)";
    Server.Post(ActorMethodsRoute, MethodNotAllowedHandler);
    Server.Put(ActorMethodsRoute, MethodNotAllowedHandler);
    Server.Patch(ActorMethodsRoute, MethodNotAllowedHandler);
    Server.Delete(ActorMethodsRoute, MethodNotAllowedHandler);
    Server.Options(ActorMethodsRoute, MethodNotAllowedHandler);

    const std::string ActorMethodRoute =
        R"(/api/v1/world/actors/([0-9]+)/methods/([a-z][a-z0-9_]{0,127}))";
    Server.Get(ActorMethodRoute, MethodNotAllowedHandler);
    Server.Put(ActorMethodRoute, MethodNotAllowedHandler);
    Server.Patch(ActorMethodRoute, MethodNotAllowedHandler);
    Server.Delete(ActorMethodRoute, MethodNotAllowedHandler);
    Server.Options(ActorMethodRoute, MethodNotAllowedHandler);

    const std::string SystemCommandRoute = R"(/api/v1/system/commands/([a-z][a-z0-9_]{0,127}))";
    Server.Get(SystemCommandRoute, MethodNotAllowedHandler);
    Server.Put(SystemCommandRoute, MethodNotAllowedHandler);
    Server.Patch(SystemCommandRoute, MethodNotAllowedHandler);
    Server.Delete(SystemCommandRoute, MethodNotAllowedHandler);
    Server.Options(SystemCommandRoute, MethodNotAllowedHandler);

    Server.set_error_handler([](const httplib::Request&, httplib::Response& Response) {
      if (Response.status == 413) {
        SetJsonResponse(
            Response,
            413,
            MakeAutomationError(EAutomationErrorCode::RequestTooLarge, RequestTooLargeMessage)
        );
        return;
      }

      SetJsonResponse(
          Response,
          Response.status == 405 ? 405 : 404,
          MakeAutomationError(
              EAutomationErrorCode::InvalidRequest,
              Response.status == 405 ? MethodNotAllowedMessage : RouteNotFoundMessage
          )
      );
    });

    Server.set_exception_handler(
        [](const httplib::Request&, httplib::Response& Response, std::exception_ptr ExceptionPtr) {
          try {
            if (ExceptionPtr) {
              std::rethrow_exception(ExceptionPtr);
            }
          } catch (const std::exception&) {
          } catch (...) {
          }
          SetJsonResponse(
              Response,
              500,
              MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)
          );
        }
    );
  }

  FAutomationConfig Config;
  FAutomationApiController& ApiController;
  httplib::Server Server;
  std::thread ServerThread;
  mutable std::mutex LifecycleMutex;
  std::atomic_bool bRunning = false;
  std::atomic_bool bStopRequested = false;
};

FAutomationHttpServer::FAutomationHttpServer(
    const FAutomationConfig& InConfig, FAutomationApiController& InApiController
)
    : ImplPtr(std::make_unique<Impl>(InConfig, InApiController)) {}

FAutomationHttpServer::~FAutomationHttpServer() { Stop(); }

bool FAutomationHttpServer::Start() {
  std::scoped_lock Lock(ImplPtr->LifecycleMutex);
  if (ImplPtr->bRunning.load()) {
    return true;
  }
  if (ImplPtr->ServerThread.joinable()) {
    ImplPtr->ServerThread.join();
  }

  if (ImplPtr->Config.BindAddress != "127.0.0.1" || ImplPtr->Config.Port == 0) {
    M_LOG(
        "Automation server refused invalid bind configuration: {}:{}",
        ImplPtr->Config.BindAddress,
        ImplPtr->Config.Port
    );
    return false;
  }

  M_LOG("Automation server starting on {}:{}", ImplPtr->Config.BindAddress, ImplPtr->Config.Port);
  const int BoundPort =
      ImplPtr->Server.bind_to_port(ImplPtr->Config.BindAddress, ImplPtr->Config.Port);
  if (BoundPort < 0) {
    M_LOG(
        "Automation server failed to bind to {}:{}",
        ImplPtr->Config.BindAddress,
        ImplPtr->Config.Port
    );
    return false;
  }

  ImplPtr->bStopRequested.store(false);
  ImplPtr->bRunning.store(true);
  try {
    ImplPtr->ServerThread = std::thread([this]() {
      const bool ListenSucceeded = ImplPtr->Server.listen_after_bind();
      ImplPtr->bRunning.store(false);
      if (!ListenSucceeded && !ImplPtr->bStopRequested.load()) {
        M_LOG("Automation server listener stopped unexpectedly.");
      }
    });
  } catch (const std::exception& Exception) {
    ImplPtr->bRunning.store(false);
    ImplPtr->Server.stop();
    M_LOG("Automation server thread creation failed: {}", Exception.what());
    return false;
  } catch (...) {
    ImplPtr->bRunning.store(false);
    ImplPtr->Server.stop();
    M_LOG("Automation server thread creation failed.");
    return false;
  }

  M_LOG("Automation server listening on {}:{}", ImplPtr->Config.BindAddress, BoundPort);
  return true;
}

void FAutomationHttpServer::StopAcceptingRequests() {
  if (!ImplPtr) {
    return;
  }
  ImplPtr->bStopRequested.store(true);
  ImplPtr->Server.stop();
}

void FAutomationHttpServer::Stop() {
  if (!ImplPtr) {
    return;
  }

  StopAcceptingRequests();
  std::scoped_lock Lock(ImplPtr->LifecycleMutex);
  if (ImplPtr->ServerThread.joinable()) {
    M_LOG("Automation server stopping.");
    ImplPtr->ServerThread.join();
    M_LOG("Automation server stopped.");
  }
  ImplPtr->bRunning.store(false);
}

bool FAutomationHttpServer::IsRunning() const { return ImplPtr && ImplPtr->bRunning.load(); }
