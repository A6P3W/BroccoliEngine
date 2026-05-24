#pragma once
#include "InputDevice.h"
#include <vector>
#include <memory>

class KeyboardDevice;
class InputManager
{
private:
	InputManager();
	~InputManager();
public:
	static InputManager& GetInstance()
	{
		static InputManager instance;
		return instance;
	}
	void AddDevice(std::unique_ptr<InputDevice> device);

	template<class T>
	T* GetDevice() const {
		for (const auto& d : m_devices) {
			if (auto* p = dynamic_cast<T*>(d.get())) return p;
		}
		return nullptr;
	}

	void Update();


private:
	std::vector<std::unique_ptr<InputDevice>> m_devices;
};
