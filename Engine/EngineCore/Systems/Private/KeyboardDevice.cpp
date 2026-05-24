#include "KeyboardDevice.h"
#include <DxLib.h>


void KeyboardDevice::Update()
{
	char tmp[256];
	GetHitKeyStateAll(tmp);
	for (int i = 0; i < 256; ++i) {
		m_prevKey[i] = m_key[i];
		m_key[i] = (tmp[i] != 0);
	}
}

bool KeyboardDevice::GetPressStart(int code) const { return !m_prevKey[code] && m_key[code]; }
bool KeyboardDevice::GetPressing(int code)   const { return  m_key[code]; }
bool KeyboardDevice::GetRelease(int code)    const { return  m_prevKey[code] && !m_key[code]; }
