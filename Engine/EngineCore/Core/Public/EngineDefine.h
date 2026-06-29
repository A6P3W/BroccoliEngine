#pragma once

constexpr bool IsDebug =
#if defined _DEBUG
    true;
#else
    false;
#endif

constexpr bool IsEditor =
#if defined _EDITOR
    true;
#else
    false;
#endif

constexpr bool IsRelease =
#if defined _RELEASE
    true;
#else
    false;
#endif

constexpr int VirtualWidth = 1920;
constexpr int VirtualHeight = 1080;
