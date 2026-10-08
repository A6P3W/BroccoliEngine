#pragma once

#include <nlohmann/json.hpp>

#include "Actor.h"
#include "AutomationAnnotations.h"

class APhysics3DQueryTestActor final : public AActor {
 private:
  static nlohmann::json ParseResult(const std::string& Result);

 public:
  DEFINE_ACTOR_CLASS(APhysics3DQueryTestActor)
  APhysics3DQueryTestActor();
  CONTROL_METHOD(
          .Name = "observe_queries",
          .Description = "Observe exact overlap queries against the query fixture.",
          .ResultAdapter = ^^APhysics3DQueryTestActor::ParseResult
  )
  std::string ObserveQueries();
  CONTROL_METHOD(
          .Name = "observe_rays",
          .Description = "Observe exact raycasts against the query fixture.",
          .ResultAdapter = ^^APhysics3DQueryTestActor::ParseResult
  )
  std::string ObserveRays();
};

class APhysics3DQueryBoxActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DQueryBoxActor)
  APhysics3DQueryBoxActor();

 protected:
  void BeginPlay() override;
};
