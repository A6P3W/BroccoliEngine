#pragma once

constexpr bool IsDebug =
#if defined _DEBUG
true;
#else
false;
#endif
