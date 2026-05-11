#include <DxLib.h>
#include "Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetGraphMode(1920, 1080, 32);
	ChangeWindowMode(false);
	DxLib_Init();
	Application App;
	App.Run();
	return 0;
}

