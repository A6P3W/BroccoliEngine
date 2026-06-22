#include "InputManager.h"

InputManager::InputManager(){}

InputManager::~InputManager()
{
}

void InputManager::AddDevice(std::unique_ptr<InputDevice> device)
{
	Devices.push_back(std::move(device));
}


void InputManager::Update()
{
	for (auto& d : Devices) d->Update();
}
