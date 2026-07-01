#include <EOS_ProductCredentials.h>

#include "EOSCoreManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
#include "SceneManager.h"

namespace {
void InitializeOnlineServices() {
  FEOSConfig Config;
  Config.ProductName = SampleConstants::GameName;
  Config.ProductVersion = "0.1.0";
  Config.ProductId = SampleConstants::ProductId;
  Config.SandboxId = SampleConstants::SandboxId;
  Config.DeploymentId = SampleConstants::DeploymentId;
  Config.ClientId = SampleConstants::ClientCredentialsId;
  Config.ClientSecret = SampleConstants::ClientCredentialsSecret;
  Config.EncryptionKey = SampleConstants::EncryptionKey;

  EOSCoreManager::Get().Initialize(Config);
}
}  // namespace

void SetupGame() {
  InitializeOnlineServices();

  auto& sceneManager = SceneManager::GetInstance();
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level1, "../Engine/EngineSide/NetworkTest/NetworkTestLevel.BLevel"
  );
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level2, "../Engine/EngineSide/NetworkTest/NetworkTestLevel2.BLevel"
  );
  sceneManager.OpenSceneById(NetworkTestSceneIds::Level1);
}