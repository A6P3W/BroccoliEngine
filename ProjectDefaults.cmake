set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Editor;Release" CACHE STRING "" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Editor Release)
endif()

function(broccoli_apply_common_msvc_options Target)
  if(MSVC)
    target_compile_options("${Target}" PRIVATE
      /W3 /sdl /MP /Zc:preprocessor /utf-8
      /wd4251 /wd4819
    )
  endif()
endfunction()

function(broccoli_configure_plugin)
  set(Options)
  set(OneValueArguments TARGET)
  cmake_parse_arguments(BroccoliPlugin "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(NOT BroccoliPlugin_TARGET)
    message(FATAL_ERROR "broccoli_configure_plugin requires TARGET.")
  endif()

  set_property(TARGET "${BroccoliPlugin_TARGET}" PROPERTY EXCLUDE_FROM_ALL TRUE)
  foreach(Configuration IN ITEMS Debug Editor Release)
    string(TOUPPER "${Configuration}" ConfigurationUpper)
    set(PluginListVariable "BROCCOLI_PLUGINS_${ConfigurationUpper}")
    if(NOT "${BroccoliPlugin_TARGET}" IN_LIST ${PluginListVariable})
      set_property(
        TARGET "${BroccoliPlugin_TARGET}"
        PROPERTY "EXCLUDE_FROM_DEFAULT_BUILD_${Configuration}" TRUE
      )
    endif()
  endforeach()
endfunction()

function(broccoli_add_configured_plugins_to_target)
  set(Options)
  set(OneValueArguments TARGET CONFIGURATION)
  cmake_parse_arguments(BroccoliPluginBuild "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(NOT BroccoliPluginBuild_TARGET OR NOT BroccoliPluginBuild_CONFIGURATION)
    message(FATAL_ERROR "broccoli_add_configured_plugins_to_target requires TARGET and CONFIGURATION.")
  endif()

  string(TOUPPER "${BroccoliPluginBuild_CONFIGURATION}" ConfigurationUpper)
  set(PluginListVariable "BROCCOLI_PLUGINS_${ConfigurationUpper}")
  foreach(Plugin IN LISTS ${PluginListVariable})
    if(TARGET "${Plugin}")
      add_dependencies("${BroccoliPluginBuild_TARGET}" "${Plugin}")
    endif()
  endforeach()
endfunction()
