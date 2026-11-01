#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "GameInstance.h"
#include "GameModeBase.h"
#include "NetMode.h"
#include "NetworkTypes.h"
#include "World.h"
class BROCCOLI_ENGINE_API SceneManager {
 public:
  struct FRegisteredLevelSnapshot {
    FNetworkSceneId SceneId = 0;
    std::string LevelPath;
  };

  static SceneManager& GetInstance();
  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;
  template <class T>
  void OpenGameMode() {
    QueueSceneFactory(MakeSceneFactory<T>(), GetCurrentNetMode(), 0);
  }

  World* GetCurrentScene();

  /// @brief 保留中のシーン切替処理の実行結果。
  struct FSceneChangeResult {
    uint64_t RequestId = 0;  ///< 処理されたリクエストID（切替要求がなかった場合は0）
    bool Success = false;    ///< シーンの生成・ロードに成功した場合はtrue
  };

  /// @brief キューに積まれているシーン切替要求を実行し、その結果を返す。
  FSceneChangeResult ProcessSceneChanges();

  /// @brief 現在保留されているシーン切替要求のリクエストIDを取得する（保留なしの場合は0）。
  uint64_t GetPendingRequestId() const;
  void Shutdown();

  void RegisterLevelPath(FNetworkSceneId SceneId, const std::string& LevelPath);
  bool OpenLevelById(FNetworkSceneId SceneId);
  bool OpenLevelById(FNetworkSceneId SceneId, ENetMode NetMode);
  bool OpenLevelByPath(const std::string& LevelPath);
  bool OpenLevelByPath(const std::string& LevelPath, ENetMode NetMode);
  bool IsSceneRegistered(FNetworkSceneId SceneId) const;
  std::vector<FRegisteredLevelSnapshot> GetRegisteredLevels() const;
  FNetworkSceneId GetCurrentSceneId() const;
  const std::string& GetCurrentLevelPath() const;
  ENetMode GetCurrentNetMode() const;
  void SetStartupLevelPath(const std::string& LevelPath);
  const std::string& GetStartupLevelPath() const;
  bool OpenStartupLevel();

  template <class T>
  void SetGameInstance() {
    SetGameInstanceInternal(std::make_unique<T>());
  }
  GameInstance* GetGameInstance();

 private:
  SceneManager();
  ~SceneManager();
  using RegisteredSceneFactory = std::function<std::unique_ptr<World>(ENetMode)>;

  template <class T>
  static RegisteredSceneFactory MakeSceneFactory() {
    return [](ENetMode NetMode) -> std::unique_ptr<World> {
      auto newWorld = std::make_unique<World>();
      newWorld->SetNetMode(NetMode);
      if (newWorld->IsServer()) {
        newWorld->SpawnGameMode<T>();
      }
      return newWorld;
    };
  }

  void QueueSceneFactory(RegisteredSceneFactory Factory, ENetMode NetMode, FNetworkSceneId SceneId);
  void QueueLevelPath(const std::string& LevelPath, ENetMode NetMode, FNetworkSceneId SceneId);
  void SetGameInstanceInternal(std::unique_ptr<GameInstance> NewGameInstance);
  struct Impl;
  Impl* ImplPtr = nullptr;
};
