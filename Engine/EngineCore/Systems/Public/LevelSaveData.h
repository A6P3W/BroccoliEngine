#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "ActorComponent.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"
#include "nlohmann/json.hpp"

struct FLevelMetaData {
  std::string GameModeClassName;
};

struct FComponentSaveData {
  std::string Name;
  std::string ClassName;
  std::string ParentName;
  FTransform3D RelativeTransform;
  bool HasSceneAttachmentData = false;
  EComponentCreationSource Source = EComponentCreationSource::Native;
  std::unordered_map<std::string, nlohmann::json> CustomProperties;
};

// 1アクタ分のデータ
struct FActorSaveData {
  std::string ClassName;
  std::string InstanceName;
  FTransform3D Transform;

  // アクタ固有のプロパティ保存用
  std::unordered_map<std::string, nlohmann::json> CustomProperties;
  std::vector<FComponentSaveData> Components;
};

class AActor;

/// @brief 低レベルなレベルデータAPI。実際にセーブデータを扱うコードだけが nlohmann/json を
/// インクルードするようにするため、LevelSerializer.h とは切り離している。
class BROCCOLI_ENGINE_API LevelSaveDataSerializer {
 public:
  static FActorSaveData CaptureActor(AActor* Actor);
  static void ApplyActor(AActor* Actor, const FActorSaveData& Data);

  static bool SaveData(
      const std::string& filePath,
      const FLevelMetaData& meta,
      const std::vector<FActorSaveData>& actors
  );
  static bool LoadData(const std::string& filePath, std::vector<FActorSaveData>& outActors);
  static bool LoadData(
      const std::string& filePath, FLevelMetaData& outMeta, std::vector<FActorSaveData>& outActors
  );
};
