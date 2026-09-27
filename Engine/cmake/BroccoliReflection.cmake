include_guard(GLOBAL)

function(broccoli_enable_reflection Target)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR "C++26 Reflection requires GCC 16 or later (-freflection).")
  endif()
  target_compile_options("${Target}" PRIVATE -freflection)
endfunction()
