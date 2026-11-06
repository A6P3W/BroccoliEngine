#include "SkeletalMeshComponent.h"

#include <algorithm>
#include <cmath>

#include "Log.h"
#include "RenderSystem.h"
#include "ResourceManager.h"

REGISTER_COMPONENT(MSkeletalMeshComponent)

namespace {
constexpr float AnimationFPS = 60.0f;
}

MSkeletalMeshComponent::~MSkeletalMeshComponent() { ReleaseModelInstance(); }

void MSkeletalMeshComponent::SetModelPath(const FPath& Path) {
  ModelPath = Path;
  SyncModelPath();
}

void MSkeletalMeshComponent::SyncModelPath() {
  if (ModelPath == LoadedPath) return;
  ReleaseModelInstance();
  LoadedPath = ModelPath;
  if (!ModelPath.Empty()) {
    ModelHandle = ResourceManager::GetInstance().CreateSkeletalMeshInstance(ModelPath);
  }
}

void MSkeletalMeshComponent::ReleaseModelInstance() {
  if (ModelHandle != 0) ResourceManager::GetInstance().DestroySkeletalMeshInstance(ModelHandle);
  ModelHandle = 0;
  LoadedPath = FPath{};
  ClipIndex = -1;
  ElapsedSeconds = 0.0;
  Stop();
}

void MSkeletalMeshComponent::OnRegister() { SyncModelPath(); }
void MSkeletalMeshComponent::OnUnregister() { ReleaseModelInstance(); }
void MSkeletalMeshComponent::OnComponentDestroy() {
  ReleaseModelInstance();
  MSceneComponent::OnComponentDestroy();
}
void MSkeletalMeshComponent::OnModelPathChanged(FPath OldValue) {
  (void)OldValue;
  SyncModelPath();
}

std::vector<std::string> MSkeletalMeshComponent::GetAnimationNames() const {
  return ResourceManager::GetInstance().GetSkeletalMeshClipNames(ModelHandle);
}

float MSkeletalMeshComponent::GetCurrentDurationSeconds() const {
  const int Count =
      ResourceManager::GetInstance().GetSkeletalMeshClipFrameCount(ModelHandle, ClipIndex);
  return Count > 1 ? static_cast<float>(Count - 1) / AnimationFPS : 0.0f;
}

bool MSkeletalMeshComponent::Play(std::string_view Name, bool Loop, bool Restart) {
  SyncModelPath();
  const auto Names = GetAnimationNames();
  const auto It = std::find(Names.begin(), Names.end(), Name);
  if (It == Names.end()) {
    M_LOG(Warning, "Unknown or unavailable skeletal clip {} in {}", Name, ModelPath.String());
    return false;
  }
  const int Index = static_cast<int>(It - Names.begin());
  if (Playing && ClipIndex == Index && !Restart) {
    Looping = Loop;
    return true;
  }
  if (!ResourceManager::GetInstance().ApplySkeletalMeshFrame(ModelHandle, Index, 0.0f))
    return false;
  ClipIndex = Index;
  ElapsedSeconds = 0.0;
  Looping = Loop;
  Paused = false;
  Finished = !Loop && GetCurrentDurationSeconds() == 0.0f;
  Playing = !Finished;
  return true;
}

void MSkeletalMeshComponent::Stop() {
  Playing = false;
  Paused = false;
  Finished = false;
}
void MSkeletalMeshComponent::Pause() {
  if (Playing) Paused = true;
}
void MSkeletalMeshComponent::Resume() {
  if (Playing) Paused = false;
}
void MSkeletalMeshComponent::SetPlaybackSpeed(float Speed) {
  if (!std::isfinite(Speed) || Speed <= 0.0f) {
    M_LOG(Warning, "Invalid skeletal playback speed {} for {}", Speed, ModelPath.String());
    return;
  }
  PlaybackSpeed = Speed;
}

void MSkeletalMeshComponent::OnUpdate(float DeltaTime) {
  MSceneComponent::OnUpdate(DeltaTime);
  SyncModelPath();
  if (!Playing || Paused || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) return;
  const double Duration = GetCurrentDurationSeconds();
  ElapsedSeconds += static_cast<double>(DeltaTime) * PlaybackSpeed;
  if (Looping) {
    ElapsedSeconds = Duration > 0.0 ? std::fmod(ElapsedSeconds, Duration) : 0.0;
  } else if (ElapsedSeconds >= Duration) {
    ElapsedSeconds = Duration;
    Finished = true;
    Playing = false;
  }
  if (!ResourceManager::GetInstance().ApplySkeletalMeshFrame(
          ModelHandle, ClipIndex, static_cast<float>(ElapsedSeconds * AnimationFPS)
      )) {
    M_LOG(Warning, "Skeletal frame update failed for {}", ModelPath.String());
    Stop();
  }
}

void MSkeletalMeshComponent::Draw() {
  SyncModelPath();
  if (!IsVisible() || !ResourceManager::GetInstance().IsModelValid(ModelHandle)) return;
  RenderSystem::GetInstance().SubmitStaticMesh(GetWorldTransform3D(), ModelHandle, Tint);
}
