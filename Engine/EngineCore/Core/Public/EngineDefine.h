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
