#include "DefaultScene.h"
#include <string>
#include "ObjectManager.h"
#include "Objects/Player.h"
#include "Objects/SampleA.h"
#include "Objects/Map.h"
#include "ObjectManager.h"
#include <Utils/Umath.h>
ADefaultScene::ADefaultScene() {
	ObjectManager::GetInstance().SpawnObject<AMap>(FVector2D::ZeroVector, 0.0f);
	ObjectManager::GetInstance().SpawnObject<APlayer>({ 0,0 }, { 0 });
}

void ADefaultScene::OnUpdate(float DeltaTime)
{
}
