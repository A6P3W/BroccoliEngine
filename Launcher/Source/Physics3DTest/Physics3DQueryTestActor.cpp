#include "Physics3DQueryTestActor.h"

#include <limits>

#include "BoxCollisionComponent3D.h"
#include "ControlMacros.h"
#include "PhysicsSystem3D.h"
#include "RigidBody3DComponent.h"
#include "SphereCollisionComponent3D.h"
#include "World.h"

REGISTER_ACTOR(APhysics3DQueryTestActor)
REGISTER_ACTOR(APhysics3DQueryBoxActor)
CONTROL_METHOD(
    "observe_queries",
    "Observe exact overlap queries against the query fixture.",
    &APhysics3DQueryTestActor::ObserveQueries,
    CONTROL_PARAMETERS(),
    ([](const std::string& Result) { return nlohmann::json::parse(Result); })
)

APhysics3DQueryTestActor::APhysics3DQueryTestActor() {
  auto* Body = NewObject<MRigidBody3DComponent>(this);
  Body->RegisterComponent();
  auto* Collider = NewObject<MSphereCollisionComponent3D>(this);
  Collider->SetRadius(0.5f);
  Collider->SetCollisionLayer3D(1);
  Collider->RegisterComponent();
}

APhysics3DQueryBoxActor::APhysics3DQueryBoxActor() {
  auto* Body = NewObject<MRigidBody3DComponent>(this);
  Body->RegisterComponent();
  auto* Collider = NewObject<MBoxCollisionComponent3D>(this);
  Collider->SetHalfExtent({2.0f, 0.2f, 0.2f});
  Collider->SetCollisionLayer3D(2);
  Collider->RegisterComponent();
}

void APhysics3DQueryBoxActor::BeginPlay() {
  SetActorLocation3D({10.0f, 0.0f, 0.0f});
  SetActorRotation3D({0.0f, 0.0f, 0.382683432f, 0.923879533f});
}

std::string APhysics3DQueryTestActor::ObserveQueries() {
  auto* Physics = GetWorld()->GetPhysicsSystem3D();
  nlohmann::json Result = nlohmann::json::object();
  auto Record = [&](const char* Name, const std::vector<FPhysicsQueryHit3D>& Hits) {
    auto Entries = nlohmann::json::array();
    for (const auto& Hit : Hits) {
      Entries.push_back(
          {{"actor_id", Hit.Actor->GetActorId()}, {"instance_name", Hit.Actor->GetInstanceName()}}
      );
    }
    Result[Name] = Entries;
  };
  Record("sphere_inside", Physics->OverlapSphere({0.99f, 0, 0}, 0.5f, {}));
  Record("sphere_outside", Physics->OverlapSphere({1.01f, 0, 0}, 0.5f, {}));
  Record("sphere_corner_miss", Physics->OverlapSphere({0.8f, 0.8f, 0}, 0.5f, {}));
  Record("box_sphere_hit", Physics->OverlapBox({0.59f, 0, 0}, {0.1f, 0.1f, 0.1f}, {}));
  Record("box_sphere_miss", Physics->OverlapBox({0.61f, 0, 0}, {0.1f, 0.1f, 0.1f}, {}));
  Record("box_sphere_corner_miss", Physics->OverlapBox({0.5f, 0.5f, 0}, {0.1f, 0.1f, 0.1f}, {}));
  Record("sphere_rotated_hit", Physics->OverlapSphere({11, 1, 0}, 0.1f, {}));
  Record("sphere_rotated_miss", Physics->OverlapSphere({11, 0, 0}, 0.1f, {}));
  Record("box_rotated_hit", Physics->OverlapBox({11, 1, 0}, {0.1f, 0.1f, 0.1f}, {}));
  Record("box_rotated_miss", Physics->OverlapBox({11, 0, 0}, {0.1f, 0.1f, 0.1f}, {}));
  for (const bool Sphere : {false, true}) {
    const std::string Prefix = Sphere ? "sphere_" : "box_";
    auto Query = [&](const FPhysicsQueryFilter3D& Filter) {
      return Sphere ? Physics->OverlapSphere({5, 0, 0}, 20, Filter)
                    : Physics->OverlapBox({5, 0, 0}, {20, 20, 20}, Filter);
    };
    Record((Prefix + "all").c_str(), Query({}));
    Record((Prefix + "mask1").c_str(), Query({2, nullptr}));
    Record((Prefix + "mask2").c_str(), Query({4, nullptr}));
    Record((Prefix + "mask0").c_str(), Query({0, nullptr}));
    Record((Prefix + "ignore").c_str(), Query({0xffff, this}));
  }
  Record("invalid_radius", Physics->OverlapSphere({}, -1, {}));
  Record("invalid_extent", Physics->OverlapBox({}, {1, 0, 1}, {}));
  Record(
      "invalid_center",
      Physics->OverlapSphere({std::numeric_limits<float>::quiet_NaN(), 0, 0}, 1, {})
  );
  return Result.dump();
}
