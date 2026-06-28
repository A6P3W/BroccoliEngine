#pragma once

#include "GameModeBase.h"
#include "NetworkTypes.h"

class APlayerController;

namespace NetworkTestSceneIds
{
	constexpr FNetworkSceneId Level1 = 1;
	constexpr FNetworkSceneId Level2 = 2;
}

class ANetworkTestGameMode : public AGameModeBase
{
public:
	DEFINE_ACTOR_CLASS(ANetworkTestGameMode)
	ANetworkTestGameMode();

	void BeginPlay() override;
	void OnUpdate(float DeltaTime) override;
	void Draw() override;
	APlayerController* OnClientConnected(FNetworkConnectionId ConnectionId) override;

protected:
	virtual const char* GetSceneName() const;
	virtual const char* GetTravelButtonText() const;
	virtual FNetworkSceneId GetTravelTargetSceneId() const;

private:
	void DrawConnectionWindow();
	void DrawStatusWindow();
	void StartListenServer();
	void ConnectAsClient();

	char ServerAddress[64] = "127.0.0.1";
	int Port = 7777;
	bool bSessionStarted = false;
	std::string StatusMessage;
};

class ANetworkTestLevel2GameMode : public ANetworkTestGameMode
{
public:
	DEFINE_ACTOR_CLASS(ANetworkTestLevel2GameMode)

protected:
	const char* GetSceneName() const override;
	const char* GetTravelButtonText() const override;
	FNetworkSceneId GetTravelTargetSceneId() const override;
};
