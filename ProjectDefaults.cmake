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
