#pragma once
#include <string>

#include "BroccoliEngineAPI.h"

struct FLevelMetaData;
class World;

class BROCCOLI_ENGINE_API LevelSerializer {
 public:
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
};
