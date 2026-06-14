#include "SceneManager.h"
#include "SampleScene01/SampleScene01.h"
void SetupGame() {
	SceneManager::GetInstance().OpenScene<ASampleScene01>();
}
