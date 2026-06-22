#include "ObjectManager.h"
#include <algorithm>
#include "Actor.h"

MObjectManager::MObjectManager()
{
}

MObjectManager::~MObjectManager() = default;

void MObjectManager::Update(float DeltaTime)
{
	// 更新対象のポインタをコピーしてループを回す
	std::vector<AActor*> tempActors;
	tempActors.reserve(Actors.size());
	for (auto& object : Actors) {
		tempActors.push_back(object.get());
	}
	for (auto* object : tempActors) {
		if (object && !object->IsPendingDestroy()) {
			if (World->IsSimulating() || object->IsEditorActor() || object->CanUpdateAnytime()) {
				object->Update(DeltaTime);
			}
		}
	}

	std::erase_if(Actors, [](const std::unique_ptr<AActor>& obj) {
		return !obj || obj->IsPendingDestroy();
		});
}

void MObjectManager::Draw()
{
	for (auto& object : Actors) {
		object->Draw();
	}
}

void MObjectManager::ClearAllObjects()
{
	Actors.clear();

}
