#include <DxLib.h>
#include "Application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetGraphMode(960, 540, 32);
	ChangeWindowMode(true);
	DxLib_Init();
	Application App;
	App.Run();
	return 0;
}
