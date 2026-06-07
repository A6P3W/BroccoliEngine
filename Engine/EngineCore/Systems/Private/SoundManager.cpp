#include "SoundManager.h"

#include <DxLib.h>
#include <algorithm>

SoundManager::~SoundManager()
{
	if (m_BGMHandle != -1) {
		StopSoundMem(m_BGMHandle);
	}

	for (const auto& sound : m_MasterSounds) {
		DeleteSoundMem(sound.second);
	}
}

int SoundManager::GetMasterHandle(const std::string& path)
{
	if (m_MasterSounds.count(path)) return m_MasterSounds[path];

	int handle = LoadSoundMem(path.c_str());
	if (handle == -1) return -1;

	m_MasterSounds[path] = handle;
	return handle;
}

int SoundManager::PlaySE(const std::string& path, bool loop)
{
	int master = GetMasterHandle(path);
	if (master == -1) return -1;

	int playHandle = DuplicateSoundMem(master);
	if (playHandle == -1) return -1;

	SetVolume(playHandle, 1.0f);

	PlaySoundMem(playHandle, loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK, TRUE);

	return playHandle;
}

int SoundManager::PlayBGM(const std::string& path, bool loop)
{
	if (m_BGMHandle != -1) {
		Stop(m_BGMHandle);
		m_BGMHandle = -1;
	}

	m_BGMHandle = PlaySE(path, loop);
	return m_BGMHandle;
}

void SoundManager::SetVolume(int handle, float volume)
{
	if (handle == -1) return;

	float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
	float clampedMasterVolume = std::clamp(m_MasterVolume, 0.0f, 1.0f);
	int vol = static_cast<int>(clampedVolume * clampedMasterVolume * 255.0f);
	ChangeVolumeSoundMem(vol, handle);
}

void SoundManager::Stop(int handle)
{
	if (handle == -1) return;

	StopSoundMem(handle);
}
