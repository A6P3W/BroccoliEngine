#pragma once

// raylib 6.0 の静的ライブラリと Win32 API の同名シンボルを分離する。
// raylib を直接 include せず、vcpkg overlay port と対になるこの境界を使用する。
#if defined(_WIN32)
#define CloseWindow BroccoliRaylibCloseWindow
#define ShowCursor BroccoliRaylibShowCursor
#define PlaySound BroccoliRaylibPlaySound
#endif

#include <raylib.h>
#include <rlgl.h>
