#include "InputMapper.h"
#include "InputManager.h"
#include <DxLib.h>

InputMapper::InputMapper() {
    keyBindings[E_INPUT_ACTION::CANCEL] = { KEY_INPUT_ESCAPE };
	keyBindings[E_INPUT_ACTION::UP] = { KEY_INPUT_UP, KEY_INPUT_W };
    keyBindings[E_INPUT_ACTION::DOWN] = { KEY_INPUT_DOWN, KEY_INPUT_S };
    keyBindings[E_INPUT_ACTION::LEFT] = { KEY_INPUT_LEFT, KEY_INPUT_A };
    keyBindings[E_INPUT_ACTION::RIGHT] = { KEY_INPUT_RIGHT, KEY_INPUT_D };
    keyBindings[E_INPUT_ACTION::MOVE] = {
    KEY_INPUT_UP, KEY_INPUT_DOWN,
    KEY_INPUT_LEFT, KEY_INPUT_RIGHT,
    KEY_INPUT_W, KEY_INPUT_S,
    KEY_INPUT_A, KEY_INPUT_D
    };
    keyBindings[E_INPUT_ACTION::INTERACT] = { KEY_INPUT_F };
}

bool InputMapper::GetKeyPressStart(E_INPUT_ACTION action) {
    for (int keyCode : keyBindings[action]) {
        if (InputManager::GetInstance().GetKeyPressStart(keyCode)) return true;
    }
    return false;
}

bool InputMapper::GetKeyPressing(E_INPUT_ACTION action) {
    for (int keyCode : keyBindings[action]) {
        if (InputManager::GetInstance().GetKeyPressing(keyCode)) return true;
    }
    return false;
}

bool InputMapper::GetKeyRelease(E_INPUT_ACTION action) {
    for (int keyCode : keyBindings[action]) {
        if (InputManager::GetInstance().GetKeyRelease(keyCode)) return true;
    }
    return false;
}
