#pragma once
#include <map>
#include <vector>
enum E_INPUT_ACTION
{
	CANCEL,
	UP,
	DOWN,
	LEFT,
	RIGHT,
	INTERACT,

};
class InputMapper {
public:
    static InputMapper& GetInstance() {
        static InputMapper instance;
        return instance;
    }

    bool GetKeyPressStart(E_INPUT_ACTION action);
    bool GetKeyPressing(E_INPUT_ACTION action);
	bool GetKeyRelease(E_INPUT_ACTION action);

private:
	InputMapper();
    std::map<E_INPUT_ACTION, std::vector<int>> keyBindings;
};
