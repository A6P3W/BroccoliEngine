#include "SoundComponent.h"

#include "Actor.h"
#include "SoundManager.h"
#include "World.h"
#include <algorithm>

int MSoundComponent::PlaySE(const std::string& path, bool loop)
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) return -1;

	int handle = soundManager->PlaySE(path, loop);
	if (handle != -1) {
		m_PlayingHandles.push_back(handle);
	}

	return handle;
}

int MSoundComponent::PlayBGM(const std::string& path, bool loop)
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) return -1;

	int handle = soundManager->PlayBGM(path, loop);
	if (handle != -1) {
		m_PlayingHandles.push_back(handle);
	}

	return handle;
}

void MSoundComponent::SetVolume(int handle, float volume)
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) return;

	soundManager->SetVolume(handle, volume);
}

void MSoundComponent::Stop(int handle)
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) return;

	soundManager->Stop(handle);
	std::erase(m_PlayingHandles, handle);
}

void MSoundComponent::StopAll()
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) {
		m_PlayingHandles.clear();
		return;
	}

	for (int handle : m_PlayingHandles) {
		soundManager->Stop(handle);
	}
	m_PlayingHandles.clear();
}

void MSoundComponent::SetMasterVolume(float volume)
{
	SoundManager* soundManager = GetSoundManager();
	if (!soundManager) return;

	soundManager->SetMasterVolume(volume);
}

void MSoundComponent::OnComponentDestroy()
{
	StopAll();
}

SoundManager* MSoundComponent::GetSoundManager() const
{
	if (!GetOwner() || !GetOwner()->GetWorld()) return nullptr;

	return GetOwner()->GetWorld()->GetSoundManager();
}
