#pragma once

#include "GameModeBase.h"
#include "NetworkTypes.h"

class APlayerController;

class ANetworkTestGameMode : public AGameModeBase
{
public:
	DEFINE_ACTOR_CLASS(ANetworkTestGameMode)
	ANetworkTestGameMode();

	void BeginPlay() override;
	void OnUpdate(float DeltaTime) override;
	void Draw() override;
	APlayerController* OnClientConnected(FNetworkConnectionId ConnectionId) override;

private:
	void DrawConnectionWindow();
	void DrawStatusWindow();
	void StartListenServer();
	void ConnectAsClient();
	void SpawnHostPlayer();

	char ServerAddress[64] = "127.0.0.1";
	int Port = 7777;
	bool bSessionStarted = false;
	bool bHostPlayerSpawned = false;
	std::string StatusMessage;
};
