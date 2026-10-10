#include "Physics3DQueryTestActor.h"

#include <limits>

#include "BoxCollision3DComponent.h"
#include "CircleCollision2DComponent.h"
#include "ControlMacros.h"
#include "PhysicsSystem3D.h"
#include "RigidBody3DComponent.h"
#include "SphereCollision3DComponent.h"
#include "World.h"

namespace {
class APhysics3DDestructionFixtureActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(APhysics3DDestructionFixtureActor)
};
}  // namespace

REGISTER_ACTOR(APhysics3DQueryTestActor)
REGISTER_ACTOR(APhysics3DQueryBoxActor)
REGISTER_CONTROL_CLASS(APhysics3DQueryTestActor)

nlohmann::json APhysics3DQueryTestActor::ParseResult(const std::string& Result) {
  return nlohmann::json::parse(Result);
}

APhysics3DQueryTestActor::APhysics3DQueryTestActor() {
  auto* Body = NewObject<MRigidBody3DComponent>(this);
  Body->RegisterComponent();
  auto* Collider = NewObject<MSphereCollision3DComponent>(this);
  Collider->SetRadius(0.5f);
  Collider->SetCollisionLayer3D(1);
  Collider->RegisterComponent();
}

APhysics3DQueryBoxActor::APhysics3DQueryBoxActor() {
  auto* Body = NewObject<MRigidBody3DComponent>(this);
  Body->RegisterComponent();
  auto* Collider = NewObject<MBoxCollision3DComponent>(this);
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

std::string APhysics3DQueryTestActor::ObserveRays() {
  auto* Physics = GetWorld()->GetPhysicsSystem3D();
  nlohmann::json Result = nlohmann::json::object();
  auto Record = [&](const char* Name, const std::vector<FPhysicsQueryHit3D>& Hits) {
    auto Entries = nlohmann::json::array();
    for (const auto& Hit : Hits) {
      Entries.push_back(
          {{"instance_name", Hit.Actor->GetInstanceName()},
           {"distance", Hit.Distance},
           {"location", {{"x", Hit.Location.X}, {"y", Hit.Location.Y}, {"z", Hit.Location.Z}}}}
      );
    }
    Result[Name] = Entries;
  };
  const FPhysicsRay3D MainRay{{-2, 0, 0}, {1, 0, 0}, 20};
  Record("all", Physics->RaycastAll(MainRay, {}));
  FPhysicsQueryHit3D Nearest;
  Result["nearest"] = Physics->RaycastNearest(MainRay, {}, Nearest)
                          ? nlohmann::json{{"instance_name", Nearest.Actor->GetInstanceName()},
                                           {"distance", Nearest.Distance},
                                           {"location", {{"x", Nearest.Location.X},
                                                         {"y", Nearest.Location.Y},
                                                         {"z", Nearest.Location.Z}}}}
                          : nlohmann::json();
  Record("mask1", Physics->RaycastAll(MainRay, {2, nullptr}));
  Record("mask2", Physics->RaycastAll(MainRay, {4, nullptr}));
  Record("ignore", Physics->RaycastAll(MainRay, {0xffff, this}));
  Record("miss", Physics->RaycastAll({{-2, 3, 0}, {1, 0, 0}, 20}, {}));
  return Result.dump();
}

std::string APhysics3DQueryTestActor::TestActorDestruction() {
  nlohmann::json Results = nlohmann::json::object();
  auto SpawnBody = [](World& TestWorld) {
    auto* Actor = TestWorld.SpawnActor<APhysics3DDestructionFixtureActor>();
    auto* Body = NewObject<MRigidBody3DComponent>(Actor);
    Body->SetBodyType(ERigidBody3DType::Dynamic);
    Body->RegisterComponent();
    auto* Collider = NewObject<MSphereCollision3DComponent>(Actor);
    Collider->SetRadius(0.5f);
    Collider->SetCollisionType3D(ECollisionType3D::Overlap);
    Collider->RegisterComponent();
    return Actor;
  };
  auto Observe = [](FPhysicsSystem3D& Physics) {
    const FPhysicsRay3D Ray{{-2, 0, 0}, {1, 0, 0}, 4};
    const FPhysicsQueryFilter3D Picking{0xffff, nullptr, EPhysicsQueryLayer3D::EditorPicking};
    return nlohmann::json{
        {"bodies", Physics.GetBodyCount()},
        {"ray_hits", Physics.RaycastAll(Ray, {}).size()},
        {"overlap_hits", Physics.OverlapSphere({}, 2, {}).size()},
        {"picking_hits", Physics.RaycastAll(Ray, Picking).size()}
    };
  };
  for (const bool Pending : {false, true}) {
    World TestWorld;
    auto* Manager = TestWorld.GetActorManager();
    auto* Physics = TestWorld.GetPhysicsSystem3D();
    auto* Actor = SpawnBody(TestWorld);
    Physics->RefreshEditorPickingBody(Actor, {});
    const FActorId Id = Actor->GetActorId();
    if (!Pending) Manager->FlushPendingActors();
    const auto Before = Observe(*Physics);
    Actor->Destroy();
    Actor->Destroy();
    const bool Deferred = Manager->FindActorByIdIncludingPendingDestroy(Id) == Actor;
    Manager->RemovePendingDestroy();
    Physics->Step(1.0f / 60.0f);
    const auto After = Observe(*Physics);
    Results[Pending ? "pending_actor" : "active_actor"] = {
        {"before", Before},
        {"after", After},
        {"passed",
         Before["ray_hits"] == 1 && Before["picking_hits"] == 1 && Deferred &&
             After["bodies"] == 0 && After["ray_hits"] == 0 && After["overlap_hits"] == 0 &&
             After["picking_hits"] == 0 &&
             Manager->FindActorByIdIncludingPendingDestroy(Id) == nullptr}
    };
  }
  {
    World TestWorld;
    auto* Physics = TestWorld.GetPhysicsSystem3D();
    auto* Actor = SpawnBody(TestWorld);
    Actor->Destroy();
    Physics->UnregisterActorBody(Actor);
    Physics->UnregisterEditorPickingBody(Actor);
    Physics->UnregisterActorBody(Actor);
    Physics->UnregisterEditorPickingBody(Actor);
    Physics->RefreshActorBody(Actor);
    Physics->RefreshEditorPickingBody(Actor, {});
    Results["pending_refresh"] = {{"passed", Physics->GetBodyCount() == 0}};
  }
  for (const bool Clear : {false, true}) {
    World TestWorld;
    auto* Manager = TestWorld.GetActorManager();
    auto* Physics = TestWorld.GetPhysicsSystem3D();
    auto* First = SpawnBody(TestWorld);
    auto* Survivor = SpawnBody(TestWorld);
    Manager->FlushPendingActors();
    auto* Pending = SpawnBody(TestWorld);
    Physics->Step(1.0f / 60.0f);
    auto* Collider = Survivor->GetComponents<MCollision3DComponent>().front();
    const auto ContactsBefore = Collider->GetOverlappingActors().size();
    if (Clear) {
      Manager->ClearAllObjects();
    } else {
      First->Destroy();
      Pending->Destroy();
      Manager->RemovePendingDestroy();
    }
    const bool ContactsCleared = Clear || Collider->GetOverlappingActors().empty();
    Physics->Step(1.0f / 60.0f);
    const auto After = Observe(*Physics);
    Results[Clear ? "clear_all_contacting" : "destroy_contacting"] = {
        {"contacts_before", ContactsBefore},
        {"after", After},
        {"passed",
         ContactsBefore == 2 && ContactsCleared && After["overlap_hits"] == (Clear ? 0 : 1)}
    };
    Manager->ClearAllObjects();
    Results[Clear ? "clear_all_contacting" : "destroy_contacting"]["passed"] =
        Results[Clear ? "clear_all_contacting" : "destroy_contacting"]["passed"].get<bool>() &&
        Physics->GetBodyCount() == 0;
  }
  {
    World TestWorld;
    auto* Physics = TestWorld.GetPhysicsSystem3D();
    auto* Manager = TestWorld.GetActorManager();
    bool Passed = true;
    for (int Index = 0; Index < 100; ++Index) {
      auto* Actor = SpawnBody(TestWorld);
      if (Index % 2 == 0) Manager->FlushPendingActors();
      Actor->Destroy();
      Manager->RemovePendingDestroy();
      Physics->Step(1.0f / 60.0f);
      Passed = Passed && Physics->GetBodyCount() == 0;
    }
    Results["spawn_destroy_100"] = {{"passed", Passed}};
    for (const bool DestroyCollider : {false, true}) {
      auto* Actor = SpawnBody(TestWorld);
      if (DestroyCollider) {
        Actor->GetComponents<MCollision3DComponent>().front()->DestroyComponent();
      } else {
        Actor->GetComponents<MRigidBody3DComponent>().front()->DestroyComponent();
      }
      Physics->Step(1.0f / 60.0f);
      Results[DestroyCollider ? "destroy_collider_component" : "destroy_body_component"] = {
          {"passed",
           !Actor->IsPendingDestroy() && Physics->GetBodyCount() == 0 &&
               Physics->OverlapSphere({}, 2, {}).empty()}
      };
      Manager->ClearAllObjects();
    }
  }
  {
    World TestWorld;
    auto* First = TestWorld.SpawnActor<APhysics3DDestructionFixtureActor>();
    auto* Second = TestWorld.SpawnActor<APhysics3DDestructionFixtureActor>();
    auto* FirstCollider = NewObject<MCircleCollision2DComponent>(First);
    auto* SecondCollider = NewObject<MCircleCollision2DComponent>(Second);
    FirstCollider->RegisterComponent();
    SecondCollider->RegisterComponent();
    FirstCollider->UpdateOverlapState(Second, true);
    SecondCollider->UpdateOverlapState(First, true);
    TestWorld.GetActorManager()->FlushPendingActors();
    const bool ContactBefore = SecondCollider->IsOverlappingActor(First);
    First->Destroy();
    TestWorld.GetActorManager()->RemovePendingDestroy();
    TestWorld.GetCollisionSystem()->UpdateCollisionMap();
    TestWorld.GetCollisionSystem()->CheckCollisions();
    Results["destroy_2d"] = {
        {"passed", ContactBefore && SecondCollider->GetOverlappingActors().empty()}
    };
  }
  // Leave contacting active and pending actors for World::~World to clean up.
  {
    World TestWorld;
    SpawnBody(TestWorld);
    TestWorld.GetActorManager()->FlushPendingActors();
    SpawnBody(TestWorld);
    TestWorld.GetPhysicsSystem3D()->Step(1.0f / 60.0f);
  }
  Results["world_teardown"] = {{"passed", true}};
  return Results.dump();
}
