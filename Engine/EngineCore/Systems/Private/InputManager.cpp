#include "InputManager.h"

InputManager::InputManager(){}

InputManager::~InputManager()
{
}

void InputManager::AddDevice(std::unique_ptr<InputDevice> device)
{
	m_devices.push_back(std::move(device));
}


void InputManager::Update()
{
	for (auto& d : m_devices) d->Update();
}
