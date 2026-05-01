#include "Gamemain.h"
#include <string>
#include <Systems/ObjectManager.h>
#include "Objects/Player.h"
#include <Objects/Camera.h>
#include "Objects/SampleA.h"
#include "Objects/Map.h"
void GameMain::BeginPlay()
{
	auto player = ObjectManager::GetInstance().SpawnObject<APlayer>(0, 0);
	auto cam = ObjectManager::GetInstance().SpawnObject<ACameraObject>();
	cam->SetTarget(player);
	cam->SetCameraView();

	auto sample = ObjectManager::GetInstance().SpawnObject<ASampleA>(0, 0);

	auto map = ObjectManager::GetInstance().SpawnObject<AMap>(0, 0);
}

void GameMain::Update(float DeltaTime)
{
}

void GameMain::Draw()
{
}
