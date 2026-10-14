#include "Animation3DTestActor.h"

#include <cmath>
#include <limits>

#include "Camera3DComponent.h"
#include "ControlMacros.h"
#include "LevelSerializer.h"
#include "PathResolver.h"
#include "ResourceManager.h"
#include "SkeletalMeshComponent.h"

REGISTER_ACTOR(AAnimation3DTestActor)
REGISTER_CONTROL_CLASS(AAnimation3DTestActor)

AAnimation3DTestActor::AAnimation3DTestActor() {
  Mesh = NewObject<MSkeletalMeshComponent>(this, "FoxMesh");
  SetRootComponent(Mesh);
  Mesh->SetModelPath(FPath("/Engine/Model/Fox.glb"));
  Mesh->RegisterComponent();
}

void AAnimation3DTestActor::BeginPlay() {
  AActor::BeginPlay();
  Mesh->SetPlaybackSpeed(Speed);
  Mesh->Play(AnimationName, true);
}

nlohmann::json AAnimation3DTestActor::GetStatus() const {
  return {
      {"names", Mesh->GetAnimationNames()},
      {"time", Mesh->GetCurrentTimeSeconds()},
      {"duration", Mesh->GetCurrentDurationSeconds()},
      {"playing", Mesh->IsPlaying()},
      {"paused", Mesh->IsPaused()},
      {"finished", Mesh->IsFinished()},
      {"speed", Mesh->GetPlaybackSpeed()},
      {"path", Mesh->GetModelPath().String()}
  };
}

nlohmann::json AAnimation3DTestActor::RunTests() {
  nlohmann::json Results = nlohmann::json::object();
  MSkeletalMeshComponent First;
  MSkeletalMeshComponent Second;
  MSkeletalMeshComponent Third;
  const FPath Fox("/Engine/Model/Fox.glb");
  First.SetModelPath(Fox);
  Second.SetModelPath(Fox);
  Third.SetModelPath(Fox);
  const auto Names = First.GetAnimationNames();
  Results["clip_names"] = Names == std::vector<std::string>{"Survey", "Walk", "Run"} &&
                          Names == Second.GetAnimationNames();
  if (Names.size() < 3) return Results;
  auto Near = [](float Left, float Right) { return std::abs(Left - Right) < 0.0001f; };
  Results["play"] =
      First.Play(Names[0], true) && Second.Play(Names[1], true) && Third.Play(Names[2], true);
  First.OnUpdate(0.25f);
  Second.SetPlaybackSpeed(2.0f);
  Second.OnUpdate(0.25f);
  Third.SetPlaybackSpeed(0.5f);
  Third.OnUpdate(0.25f);
  Results["independent_times"] = Near(First.GetCurrentTimeSeconds(), 0.25f) &&
                                 Near(Second.GetCurrentTimeSeconds(), 0.5f) &&
                                 Near(Third.GetCurrentTimeSeconds(), 0.125f);
  First.Play(Names[0], true);
  Results["repeated_play"] = Near(First.GetCurrentTimeSeconds(), 0.25f);
  First.SetModelPath(Fox);
  Results["same_path"] = Near(First.GetCurrentTimeSeconds(), 0.25f) && First.IsPlaying();
  First.Pause();
  First.OnUpdate(1.0f);
  Results["pause"] =
      First.IsPlaying() && First.IsPaused() && Near(First.GetCurrentTimeSeconds(), 0.25f);
  First.Resume();
  First.OnUpdate(0.25f);
  Results["resume"] = !First.IsPaused() && Near(First.GetCurrentTimeSeconds(), 0.5f);
  First.SetPlaybackSpeed(0.0f);
  First.SetPlaybackSpeed(-1.0f);
  First.SetPlaybackSpeed(std::numeric_limits<float>::infinity());
  First.SetPlaybackSpeed(std::numeric_limits<float>::quiet_NaN());
  Results["invalid_speed"] = First.GetPlaybackSpeed() == 1.0f;
  First.OnUpdate(-1.0f);
  First.OnUpdate(0.0f);
  First.OnUpdate(std::numeric_limits<float>::infinity());
  First.OnUpdate(std::numeric_limits<float>::quiet_NaN());
  Results["invalid_delta"] = Near(First.GetCurrentTimeSeconds(), 0.5f);
  Results["unknown_clip"] =
      !First.Play("Missing") && First.IsPlaying() && Near(First.GetCurrentTimeSeconds(), 0.5f);
  First.Play(Names[0], false);
  First.OnUpdate(First.GetCurrentDurationSeconds());
  Results["change_loop_mode"] = First.IsFinished() && !First.IsPlaying();
  First.Play(Names[0], true, true);
  First.OnUpdate(0.5f);
  First.Stop();
  First.OnUpdate(1.0f);
  Results["stop"] =
      !First.IsPlaying() && !First.IsFinished() && Near(First.GetCurrentTimeSeconds(), 0.5f);
  First.Play(Names[1], false);
  Results["switch"] = First.GetCurrentTimeSeconds() == 0.0f;
  const float Duration = First.GetCurrentDurationSeconds();
  First.OnUpdate(Duration + 0.5f);
  First.OnUpdate(1.0f);
  Results["finish"] =
      First.IsFinished() && !First.IsPlaying() && Near(First.GetCurrentTimeSeconds(), Duration);
  First.Play(Names[1], true, true);
  First.OnUpdate(Duration * 3.0f + 0.25f);
  Results["loop"] =
      First.IsPlaying() && !First.IsFinished() && Near(First.GetCurrentTimeSeconds(), 0.25f);
  First.Play(Names[1], true, true);
  Results["restart"] = First.GetCurrentTimeSeconds() == 0.0f;
  for (const int FPS : {30, 60, 144}) {
    First.Play(Names[1], false, true);
    const int Steps = static_cast<int>(std::ceil(Duration * FPS));
    for (int Step = 0; Step < Steps - 1; ++Step) First.OnUpdate(1.0f / FPS);
    const bool Before = First.IsPlaying() && !First.IsFinished();
    First.OnUpdate(1.0f / FPS + 0.00001f);
    Results["fps_" + std::to_string(FPS)] =
        Before && First.IsFinished() && Near(First.GetCurrentTimeSeconds(), Duration);
  }
  First.SetModelPath(FPath{});
  Results["empty_path"] = First.GetAnimationNames().empty() && !First.IsPlaying() &&
                          !First.IsFinished() && !First.Play(Names[0]);
  First.SetModelPath(FPath("/Engine/Model/DoesNotExist.glb"));
  Results["missing_path"] = First.GetAnimationNames().empty() && !First.Play(Names[0]);
  auto& Resources = ResourceManager::GetInstance();
  const int Static = Resources.LoadResourceModel(Fox);
  const int Instance = Resources.CreateSkeletalMeshInstance(Fox);
  Resources.DestroySkeletalMeshInstance(Static);
  Results["static_handle"] =
      Static != Instance && Resources.IsModelValid(Static) && Resources.IsModelValid(Instance);
  Resources.DestroySkeletalMeshInstance(Instance);
  Resources.DestroySkeletalMeshInstance(Instance);
  Results["destroyed_handle"] =
      !Resources.IsModelValid(Instance) && !Resources.ApplySkeletalMeshFrame(Instance, 0, 0.0f);
  Results["invalid_frame"] = !Resources.ApplySkeletalMeshFrame(Static, 0, 0.0f) &&
                             !Resources.ApplySkeletalMeshFrame(0, -1, 0.0f);
  First.SetModelPath(Fox);
  First.Play(Names[0], true);
  Mesh->UnRegisterComponent();
  Results["unregister"] = Mesh->GetAnimationNames().empty();
  Mesh->RegisterComponent();
  Results["register"] = Mesh->GetAnimationNames() == Names;
  for (int Iteration = 0; Iteration < 32; ++Iteration) {
    First.SetModelPath(FPath{});
    First.SetModelPath(Fox);
    First.Play(Names[Iteration % 3], true);
    First.OnUpdate(0.1f);
  }
  Results["reload"] = First.GetAnimationNames() == Names && First.IsPlaying();
  FActorSaveData Saved = LevelSerializer::CaptureActor(this);
  const FPath Original = Mesh->GetModelPath();
  Mesh->SetModelPath(FPath{});
  LevelSerializer::ApplyActor(this, Saved);
  Mesh->OnUpdate(0.0f);
  Results["native_restore"] =
      Mesh->GetModelPath() == Original && Mesh->GetAnimationNames() == Names;
  Mesh->Play(AnimationName, true);
  return Results;
}

nlohmann::json AAnimation3DTestActor::TestFixtures(std::string Directory) {
  nlohmann::json Results = nlohmann::json::object();
  MSkeletalMeshComponent Test;
  Test.SetModelPath(FPath(Directory + "/BindPose.glb"));
  Results["bind_pose"] = Test.GetAnimationNames().empty() && !Test.Play("Walk");
  auto& Resources = ResourceManager::GetInstance();
  const int BindPose = Resources.CreateSkeletalMeshInstance(FPath(Directory + "/BindPose.glb"));
  Results["bind_pose_handle"] = Resources.IsModelValid(BindPose);
  Resources.DestroySkeletalMeshInstance(BindPose);
  const int Skinless = Resources.CreateSkeletalMeshInstance(FPath(Directory + "/Skinless.glb"));
  Results["skinless"] = Skinless == 0;
  Test.SetModelPath(FPath(Directory + "/Names.glb"));
  Results["unique_names"] =
      Test.GetAnimationNames() == std::vector<std::string>{"Animation_0", "Walk", "Walk_1"};
  Test.SetModelPath(FPath(Directory + "/Single.glb"));
  Results["single_frame"] = Test.Play("Single", false) && Test.IsFinished() && !Test.IsPlaying() &&
                            Test.GetCurrentDurationSeconds() == 0.0f;
  Test.Play("Single", true);
  Test.OnUpdate(1.0f);
  Results["single_frame_loop"] = Test.IsPlaying() && Test.GetCurrentTimeSeconds() == 0.0f;
  Test.SetModelPath(FPath(Directory + "/キツネ.GLB"));
  Results["unicode_uppercase"] =
      Test.GetAnimationNames() == std::vector<std::string>{"Survey", "Walk", "Run"};
  Test.SetModelPath(FPath(Directory + "/Fox.gltf"));
  Results["gltf"] = Test.GetAnimationNames() == std::vector<std::string>{"Survey", "Walk", "Run"} &&
                    Test.Play("Walk");
  const int StaticSkinless = Resources.LoadResourceModel(FPath(Directory + "/Skinless.glb"));
  Results["skinless_static"] = Resources.IsModelValid(StaticSkinless);
  Test.SetModelPath(FPath(Directory + "/Fox.bin"));
  Results["unsupported_extension"] = Test.GetAnimationNames().empty() && !Test.Play("Walk");
  const int Handle = Resources.CreateSkeletalMeshInstance(FPath("/Engine/Model/Fox.glb"));
  Results["invalid_frames"] =
      !Resources.ApplySkeletalMeshFrame(Handle, -1, 0.0f) &&
      !Resources.ApplySkeletalMeshFrame(Handle, 100, 0.0f) &&
      !Resources.ApplySkeletalMeshFrame(Handle, 0, -1.0f) &&
      !Resources.ApplySkeletalMeshFrame(Handle, 0, std::numeric_limits<float>::quiet_NaN()) &&
      !Resources.ApplySkeletalMeshFrame(Handle, 0, std::numeric_limits<float>::infinity());
  Resources.DestroySkeletalMeshInstance(Handle);
  FActorSaveData Saved = LevelSerializer::CaptureActor(this);
  FComponentSaveData Instance;
  Instance.Name = "RestoredFox";
  Instance.ClassName = "MSkeletalMeshComponent";
  Instance.Source = EComponentCreationSource::Instance;
  Instance.HasSceneAttachmentData = true;
  Instance.ParentName = "FoxMesh";
  Instance.CustomProperties["ModelPath"] = "/Engine/Model/Fox.glb";
  Saved.Components.push_back(Instance);
  const std::string SavePath = PathResolver::Resolve(Directory + "/RoundTrip.BLevel");
  std::vector<FActorSaveData> Loaded;
  Results["blevel_roundtrip"] =
      LevelSerializer::SaveData(SavePath, {"AAnimation3DTestGameMode"}, {Saved}) &&
      LevelSerializer::LoadData(SavePath, Loaded) && Loaded.size() == 1;
  if (Loaded.size() == 1) {
    Mesh->SetModelPath(FPath{});
    LevelSerializer::ApplyActor(this, Loaded[0]);
    // Draw must synchronize restored native properties before the first update.
    Mesh->Draw();
    auto* Restored = dynamic_cast<MSkeletalMeshComponent*>(FindComponentByName("RestoredFox"));
    Results["native_draw_restore"] = Mesh->GetAnimationNames().size() == 3;
    Results["instance_restore"] = Restored != nullptr && Restored->IsRegistered() &&
                                  Restored->GetAnimationNames().size() == 3 &&
                                  Restored->Play("Run", true);
    if (Restored != nullptr) Restored->DestroyComponent();
    Mesh->Play(AnimationName, true);
  }
  return Results;
}

REGISTER_ACTOR(AAnimation3DTestCameraActor)

AAnimation3DTestCameraActor::AAnimation3DTestCameraActor() {
  auto* Camera = NewObject<MCamera3DComponent>(this, "AnimationCamera");
  SetRootComponent(Camera);
  Camera->SetActiveCamera();
  Camera->RegisterComponent();
}
