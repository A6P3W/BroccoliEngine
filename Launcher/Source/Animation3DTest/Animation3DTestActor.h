#pragma once
#include <nlohmann/json.hpp>

#include "Actor.h"
#include "AutomationAnnotations.h"

class MSkeletalMeshComponent;

class AAnimation3DTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAnimation3DTestActor)
  AAnimation3DTestActor();
  CONTROL_METHOD(.Name = "get_status", .Description = "Returns skeletal playback state.")
  nlohmann::json GetStatus() const;
  CONTROL_METHOD(.Name = "run_tests", .Description = "Runs skeletal playback API regression tests.")
  nlohmann::json RunTests();
  CONTROL_PARAMETER(.Index = 0, .Name = "directory")
  CONTROL_METHOD(.Name = "test_fixtures", .Description = "Tests generated GLB/glTF edge cases.")
  nlohmann::json TestFixtures(std::string Directory);

 protected:
  void BeginPlay() override;

 private:
  MSkeletalMeshComponent* Mesh = nullptr;
  EDITOR_PROPERTY()
  std::string AnimationName = "Walk";
  EDITOR_PROPERTY()
  float Speed = 1.0f;
};

class AAnimation3DTestCameraActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AAnimation3DTestCameraActor)
  AAnimation3DTestCameraActor();
};
