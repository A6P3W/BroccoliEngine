#pragma once

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

	bool GetKeyPressStart(int keyCode);
	bool GetKeyPressing(int keyCode);
	bool GetKeyRelease(int keyCode);
	bool GetMouseWheelUp();
	bool GetMouseWheelDown();
	void Update();

	bool key[256];
	bool prevKey[256];
	int mouseWheel;
};
