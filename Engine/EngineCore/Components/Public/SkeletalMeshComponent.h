#pragma once
#include <string_view>
#include <vector>

#include "Color.h"
#include "FPath.h"
#include "SceneComponent.h"

class BROCCOLI_ENGINE_API MSkeletalMeshComponent : public MSceneComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSkeletalMeshComponent)
  ~MSkeletalMeshComponent() override;
  void SetModelPath(const FPath& Path);
  const FPath& GetModelPath() const { return ModelPath; }
  std::vector<std::string> GetAnimationNames() const;
  bool Play(std::string_view Name, bool Loop = true, bool Restart = false);
  void Stop();
  void Pause();
  void Resume();
  bool IsPlaying() const { return Playing; }
  bool IsPaused() const { return Paused; }
  bool IsFinished() const { return Finished; }
  void SetPlaybackSpeed(float Speed);
  float GetPlaybackSpeed() const { return PlaybackSpeed; }
  float GetCurrentTimeSeconds() const { return static_cast<float>(ElapsedSeconds); }
  float GetCurrentDurationSeconds() const;
  void SetTint(const FColor& NewTint) { Tint = NewTint; }
  const FColor& GetTint() const { return Tint; }
  void OnUpdate(float DeltaTime) override;
  void Draw() override;

 protected:
  void OnRegister() override;
  void OnUnregister() override;
  void OnComponentDestroy() override;

 private:
  void SyncModelPath();
  void ReleaseModelInstance();
  void OnModelPathChanged(FPath OldValue);
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^MSkeletalMeshComponent::OnModelPathChanged,
          .PathFilter = EPathFilter::Model
  )
  FPath ModelPath;
  FPath LoadedPath;
  int ModelHandle = 0;
  int ClipIndex = -1;
  double ElapsedSeconds = 0.0;
  float PlaybackSpeed = 1.0f;
  bool Playing = false;
  bool Paused = false;
  bool Finished = false;
  bool Looping = true;
  FColor Tint = FColor::White;
};
