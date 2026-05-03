#include "Gamemain.h"
#include <string>
#include <Systems/ObjectManager.h>
#include "Objects/Player.h"
#include "Objects/SampleA.h"
#include "Objects/Map.h"
#include <Utils/Umath.h>
void GameMain::BeginPlay()
{
	auto player = ObjectManager::GetInstance().SpawnObject<APlayer>({100,100}, {0});

	auto sample = ObjectManager::GetInstance().SpawnObject<ASampleA>();

	auto map = ObjectManager::GetInstance().SpawnObject<AMap>({100,0}, {0});
}

void GameMain::Update(float DeltaTime)
{
}

void GameMain::Draw()
{
}
	