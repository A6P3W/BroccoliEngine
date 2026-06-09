#pragma once

#include "ActorComponent.h"
#include <string>
#include <vector>

class SoundManager;

class MSoundComponent : public MActorComponent
{
public:
	int PlaySE(const std::string& path, bool loop = false);
	int PlayBGM(const std::string& path, bool loop = true);

	void SetVolume(int handle, float volume);
	void Stop(int handle);
	void StopAll();
	void SetMasterVolume(float volume);

protected:
	void OnComponentDestroy() override;

private:
	SoundManager* GetSoundManager() const;

	std::vector<int> m_PlayingHandles;
};
