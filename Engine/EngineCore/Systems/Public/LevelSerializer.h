#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "ActorComponent.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"
#include "nlohmann/json.hpp"

/// @brief レベルのメタデータ
struct FLevelMetaData {
  std::string GameModeClassName;
};

/// @brief 1コンポーネント分の保存データ
struct FComponentSaveData {
  std::string Name;
  std::string ClassName;
  std::string ParentName;
  FTransform3D RelativeTransform;
  bool HasSceneAttachmentData = false;
  EComponentCreationSource Source = EComponentCreationSource::Native;
  // コンポーネント固有のプロパティ保存用
  std::unordered_map<std::string, nlohmann::json> CustomProperties;
};

/// @brief 1アクタ分の保存データ
struct FActorSaveData {
  std::string ClassName;
  std::string InstanceName;
  FTransform3D Transform;

  // アクタ固有のプロパティ保存用
  std::unordered_map<std::string, nlohmann::json> CustomProperties;
  std::vector<FComponentSaveData> Components;
};

class World;
class AActor;

class BROCCOLI_ENGINE_API LevelSerializer {
 public:
  /// @brief アクタをSaveData形式で取得する。
  /// @param Actor
  /// @return
  static FActorSaveData CaptureActor(AActor* Actor);
  /// @brief アクタにSaveData形式で保存された情報を適用する。
  /// @param Actor
  /// @param Data
  static void ApplyActor(AActor* Actor, const FActorSaveData& Data);
  // 現在ActorManagerにいるActorRegistryに登録されている全アクタを保存
  static bool Save(World* world, const std::string& filePath, const std::string& gameModeClassName);

  // JSONからアクタをスポーン（現シーンはクリアしない）
  static bool Load(World* world, const std::string& filePath);
  static bool Load(
      World* world,
      const std::string& filePath,
      bool bLoadGameMode,
      FLevelMetaData* outMeta = nullptr
  );

  // 低レベルAPI（EditorModeから直接データを渡す場合）
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
