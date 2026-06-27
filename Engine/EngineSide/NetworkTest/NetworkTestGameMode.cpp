#include "NetworkTest/NetworkTestGameMode.h"

#include "NetworkManager.h"
#include "NetworkTest/NetworkTestPawn.h"
#include "PlayerController.h"
#include "World.h"

#include <imgui.h>

REGISTER_GAME_MODE(ANetworkTestGameMode)
REGISTER_GAME_MODE(ANetworkTestLevel2GameMode)

ANetworkTestGameMode::ANetworkTestGameMode()
{
	StatusMessage = "Select network role.";
}

void ANetworkTestGameMode::BeginPlay()
{
	SetUpdateableAnytime(true);
	StatusMessage = std::string(GetSceneName()) + " loaded.";
}

void ANetworkTestGameMode::OnUpdate(float DeltaTime)
{
	(void)DeltaTime;

	if (GetWorld()->IsListenServer() && !bHostPlayerSpawned) {
		SpawnHostPlayer();
	}
}

void ANetworkTestGameMode::Draw()
{
	AGameModeBase::Draw();

	DrawConnectionWindow();
	DrawStatusWindow();
}

APlayerController* ANetworkTestGameMode::OnClientConnected(FNetworkConnectionId ConnectionId)
{
	const float offset = static_cast<float>(ConnectionId) * 80.0f;
	APlayerController* controller = SpawnPlayer<ANetworkTestPawn, APlayerController>({ offset, 120.0f }, static_cast<int>(ConnectionId));
	if (controller && controller->GetPawn()) {
		APawn* pawn = controller->GetPawn();
		pawn->OwnerConnectionId = ConnectionId;
		pawn->bReplicates = true;
		pawn->bHasAuthority = true;
		pawn->bIsLocallyControlled = false;
	}

	StatusMessage = "Client connected: " + std::to_string(ConnectionId);
	return controller;
}

void ANetworkTestGameMode::DrawConnectionWindow()
{
	ImGui::Begin("Network Test");
	ImGui::TextUnformatted("Local Network Replication Test");
	ImGui::Separator();

	ImGui::InputInt("Port", &Port);
	if (Port < 1) {
		Port = 1;
	}
	if (Port > 65535) {
		Port = 65535;
	}

	if (ImGui::Button("Start Listen Server")) {
		StartListenServer();
	}

	ImGui::InputText("Server IP", ServerAddress, sizeof(ServerAddress));
	if (ImGui::Button("Connect as Client")) {
		ConnectAsClient();
	}

	ImGui::Separator();
	ImGui::Text("Current Scene: %s", GetSceneName());
	if (GetWorld()->IsServer() && ImGui::Button(GetTravelButtonText())) {
		if (GetWorld()->ServerTravel(GetTravelTargetSceneId())) {
			StatusMessage = "Server travel requested.";
		}
		else {
			StatusMessage = "Server travel failed. Scene is not registered or local role is not server.";
		}
	}
	ImGui::Separator();
	ImGui::TextWrapped("%s", StatusMessage.c_str());
	ImGui::End();
}

void ANetworkTestGameMode::DrawStatusWindow()
{
	ImGui::Begin("Network Status");

	const char* modeText = "Standalone";
	if (GetWorld()->IsListenServer()) {
		modeText = "ListenServer";
	}
	else if (GetWorld()->IsClient()) {
		modeText = "Client";
	}

	NetworkManager& network = NetworkManager::GetInstance();
	ImGui::Text("Mode: %s", modeText);
	ImGui::Text("Network running: %s", network.IsRunning() ? "true" : "false");
	ImGui::Text("Local ConnectionId: %u", network.GetLocalConnectionId());
	ImGui::Text("Host player spawned: %s", bHostPlayerSpawned ? "true" : "false");
	ImGui::End();
}

void ANetworkTestGameMode::StartListenServer()
{
	if (NetworkManager::GetInstance().StartServer(static_cast<uint16_t>(Port))) {
		GetWorld()->SetNetMode(ENetMode::ListenServer);
		bSessionStarted = true;
		StatusMessage = "Listen server started. Host player will spawn on next update.";
	}
	else {
		StatusMessage = "Failed to start listen server.";
	}
}

void ANetworkTestGameMode::ConnectAsClient()
{
	if (NetworkManager::GetInstance().ConnectToServer(ServerAddress, static_cast<uint16_t>(Port))) {
		GetWorld()->SetNetMode(ENetMode::Client);
		bSessionStarted = true;
		StatusMessage = "Connecting to server...";
	}
	else {
		StatusMessage = "Failed to connect to server.";
	}
}

void ANetworkTestGameMode::SpawnHostPlayer()
{
	if (bHostPlayerSpawned) {
		return;
	}

	APlayerController* controller = SpawnPlayer<ANetworkTestPawn, APlayerController>({ 0.0f, 0.0f }, 0);
	if (controller && controller->GetPawn()) {
		APawn* pawn = controller->GetPawn();
		pawn->OwnerConnectionId = 0;
		pawn->bReplicates = true;
		pawn->bHasAuthority = true;
		pawn->bIsLocallyControlled = true;
	}

	bHostPlayerSpawned = true;
}

const char* ANetworkTestGameMode::GetSceneName() const
{
	return "NetworkTest Level 1";
}

const char* ANetworkTestGameMode::GetTravelButtonText() const
{
	return "Server Travel to Level 2";
}

FNetworkSceneId ANetworkTestGameMode::GetTravelTargetSceneId() const
{
	return NetworkTestSceneIds::Level2;
}

const char* ANetworkTestLevel2GameMode::GetSceneName() const
{
	return "NetworkTest Level 2";
}

const char* ANetworkTestLevel2GameMode::GetTravelButtonText() const
{
	return "Server Travel to Level 1";
}

FNetworkSceneId ANetworkTestLevel2GameMode::GetTravelTargetSceneId() const
{
	return NetworkTestSceneIds::Level1;
}
