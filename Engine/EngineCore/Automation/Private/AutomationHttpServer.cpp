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

void SetJsonResponse(httplib::Response& Response, int StatusCode, const nlohmann::json& Body) {
  Response.status = StatusCode;
  Response.set_header("Cache-Control", "no-store");
  Response.set_content(Body.dump(), std::string(JsonContentType));
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
        "/api/v1/world/actors", [this](const httplib::Request&, httplib::Response& Response) {
          try {
            const FAutomationHttpResponse ApiResponse = ApiController.GetWorldActors();
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
        R"(/api/v1/world/actors/(.*))",
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
    for (const std::string& Route :
         {std::string("/api/v1/world/actors"), std::string(R"(/api/v1/world/actors/(.*))")}) {
      Server.Post(Route, MethodNotAllowedHandler);
      Server.Put(Route, MethodNotAllowedHandler);
      Server.Patch(Route, MethodNotAllowedHandler);
      Server.Delete(Route, MethodNotAllowedHandler);
      Server.Options(Route, MethodNotAllowedHandler);
    }

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
