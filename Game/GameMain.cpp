#include "Gamemain.h"
#include <string>
#include <Systems/ObjectManager.h>
#include "Objects/Player.h"
#include <Objects/Camera.h>
#include "Objects/SampleA.h"
#include "Objects/Map.h"
#include <Utils/Umath.h>
void GameMain::BeginPlay()
{
	auto player = ObjectManager::GetInstance().SpawnObject<APlayer>({100,100}, {0});
	auto cam = ObjectManager::GetInstance().SpawnObject<ACameraObject>();
	cam->SetTarget(player);
	cam->SetCameraView();

	auto sample = ObjectManager::GetInstance().SpawnObject<ASampleA>();

	auto map = ObjectManager::GetInstance().SpawnObject<AMap>({10000,0}, {0});
}

void GameMain::Update(float DeltaTime)
{
}

void GameMain::Draw()
{
}
