#include <DxLib.h>
#include "InputManager.h"

InputManager::InputManager()
{
	for (int i = 0; i < 256; i++)
	{
     key[i] = false;
		prevKey[i] = false;
	}
}

InputManager::~InputManager()
{
}

void InputManager::Update()
{
	char tmpKey[256];
	GetHitKeyStateAll(tmpKey);
	for (int i = 0; i < 256; i++)
	{
       prevKey[i] = key[i];
		key[i] = (tmpKey[i] != 0);
	}
}

bool InputManager::GetKeyPressStart(int keyCode)
{
  return (!prevKey[keyCode] && key[keyCode]);
}

bool InputManager::GetKeyPressing(int keyCode)
{
    return key[keyCode];
}

bool InputManager::GetKeyRelease(int keyCode)
{
   return (prevKey[keyCode] && !key[keyCode]);
}
