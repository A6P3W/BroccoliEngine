#include "NetworkTestGameMode.h"

#include "../Common/LauncherPlayerController.h"
#include "NetworkTestPawn.h"
#include "PlayerController.h"
#include "World.h"

REGISTER_GAME_MODE(ANetworkTestGameMode)
REGISTER_GAME_MODE(ANetworkTestLevel2GameMode)

ANetworkTestGameMode::ANetworkTestGameMode() {
  SetDefaultPawnClass(ANetworkTestPawn::StaticClassName());
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}

void ANetworkTestGameMode::BeginPlay() { SetUpdateableAnytime(true); }

APlayerController* ANetworkTestGameMode::OnClientConnected(FNetworkConnectionId ConnectionId) {
  APlayerController* controller = AGameModeBase::OnClientConnected(ConnectionId);
  if (GetWorld()) {
    if (APlayerController* localPC = GetWorld()->GetOrCreateLocalPlayerController()) {
      if (ANetworkTestPawn* localPawn = dynamic_cast<ANetworkTestPawn*>(localPC->GetPawn())) {
        localPawn->SetStatusMessage("Client connected: " + std::to_string(ConnectionId));
      }
    }
  }
  return controller;
}

const char* ANetworkTestGameMode::GetSceneName() const { return "NetworkT1"; }

const char* ANetworkTestGameMode::GetTravelButtonText() const { return "Server Travel to Level 2"; }

FNetworkSceneId ANetworkTestGameMode::GetTravelTargetSceneId() const {
  return NetworkTestSceneIds::Level2;
}

const char* ANetworkTestGameMode::GetTravelTargetLevelPath() const {
  return NetworkTestLevelPaths::Level2;
}

const char* ANetworkTestLevel2GameMode::GetSceneName() const { return "Network2"; }

const char* ANetworkTestLevel2GameMode::GetTravelButtonText() const {
  return "Server Travel to Level 1";
}

FNetworkSceneId ANetworkTestLevel2GameMode::GetTravelTargetSceneId() const {
  return NetworkTestSceneIds::Level1;
}

const char* ANetworkTestLevel2GameMode::GetTravelTargetLevelPath() const {
  return NetworkTestLevelPaths::Level1;
}
