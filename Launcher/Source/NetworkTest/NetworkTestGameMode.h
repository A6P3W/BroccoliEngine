#pragma once

#include "GameModeBase.h"
#include "NetworkTypes.h"

class APlayerController;

namespace NetworkTestSceneIds {
constexpr FNetworkSceneId Level1 = 1;
constexpr FNetworkSceneId Level2 = 2;
}  // namespace NetworkTestSceneIds

namespace NetworkTestLevelPaths {
constexpr const char* Level1 = "Resources/Network1.BLevel";
constexpr const char* Level2 = "Resources/Network2.BLevel";
}  // namespace NetworkTestLevelPaths

// ネットワーク同期検証用のメインゲームモードクラス AGameModeBase を継承
class ANetworkTestGameMode : public AGameModeBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ANetworkTestGameMode)
  ANetworkTestGameMode();

  void BeginPlay() override;

  // 新しいマルチプレイ接続（クライアント）が接続された際にサーバー側で呼び出されるコールバック関数
  APlayerController* OnClientConnected(FNetworkConnectionId ConnectionId) override;

  virtual const char* GetSceneName() const;
  virtual const char* GetTravelButtonText() const;
  virtual FNetworkSceneId GetTravelTargetSceneId() const;
  virtual const char* GetTravelTargetLevelPath() const;
};

// ネットワークテストのレベル2で稼働する派生ゲームモードクラス
class ANetworkTestLevel2GameMode : public ANetworkTestGameMode {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ANetworkTestLevel2GameMode)

 protected:
  const char* GetSceneName() const override;
  const char* GetTravelButtonText() const override;
  FNetworkSceneId GetTravelTargetSceneId() const override;
  const char* GetTravelTargetLevelPath() const override;
};
