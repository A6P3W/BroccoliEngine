#include <DxLib.h>
#include "Application.h"

#if defined(_DEBUG) || defined(_EDITOR)
#include <Windows.h>
#include <cstdio>

namespace
{
	void OpenDebugConsole()
	{
		if (!AllocConsole()) {
			return;
		}

		FILE* stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
		freopen_s(&stream, "CONIN$", "r", stdin);
		SetConsoleTitleA("BroccoliEngine Debug Console");
	}
}
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#if defined(_DEBUG) || defined(_EDITOR)
	OpenDebugConsole();
#endif
	Application App;
	App.Run();
	return 0;
}
