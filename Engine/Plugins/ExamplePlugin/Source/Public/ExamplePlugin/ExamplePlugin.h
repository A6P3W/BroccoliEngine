#pragma once

#if defined(_WIN32)
#if defined(EXAMPLE_PLUGIN_BUILD)
#define EXAMPLE_PLUGIN_API __declspec(dllexport)
#else
#define EXAMPLE_PLUGIN_API __declspec(dllimport)
#endif
#else
#define EXAMPLE_PLUGIN_API
#endif

namespace ExamplePlugin {
EXAMPLE_PLUGIN_API int GetRandomNumber();
}
