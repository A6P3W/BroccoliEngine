cmake_minimum_required(VERSION 4.2)

# This file is the BroccoliEngine repository's standalone CMake entry point and
# is also included directly by generated game workspaces.
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH
    "Directory containing the BroccoliEngine vcpkg manifest")

if(NOT DEFINED BROCCOLI_OUTPUT_ROOT)
  set(BROCCOLI_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(BroccoliEngine LANGUAGES CXX)
  include("${CMAKE_CURRENT_LIST_DIR}/../ProjectDefaults.cmake")
endif()

file(GLOB_RECURSE BROCCOLI_ENGINE_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineSide/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineSide/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineSide/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/EngineSide/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.hpp"
)

# ThirdParty is intentionally not globbed recursively. It may contain
# alternative implementations, samples, or platform-specific sources.
list(APPEND BROCCOLI_ENGINE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/rlImGui/rlImGui.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/rlImGui/rlImGui.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/rlImGui/imgui_impl_raylib.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/nlohmann/json.hpp"
)

add_library(BroccoliEngine SHARED ${BROCCOLI_ENGINE_FILES})
add_library(Broccoli::Engine ALIAS BroccoliEngine)

target_compile_features(BroccoliEngine PUBLIC cxx_std_20)
target_compile_definitions(BroccoliEngine
  PRIVATE
    BROCCOLI_ENGINE_BUILD
    NO_FONT_AWESOME
    _CONSOLE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Editor>:_EDITOR>
    $<$<CONFIG:Release>:_RELEASE>
    $<$<CONFIG:Editor>:BROCCOLI_ENGINE_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources/">
)

target_include_directories(BroccoliEngine
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Plugin/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Automation/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Core/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Systems/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Components/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Network/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/GameFramework/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Utils/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineCore/Online/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineSide>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EngineSide/Default/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Editor/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/EOS/SDK/Include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../PrivateConfig
)

if(MSVC)
  broccoli_apply_common_msvc_options(BroccoliEngine)
  set_property(TARGET BroccoliEngine PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
endif()

# vcpkg packages used by the engine. Target-name fallbacks cover common
# package config names across vcpkg revisions.
find_package(cpr CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(unofficial-enet CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-minizip CONFIG REQUIRED)

function(broccoli_link_first_available Target Visibility)
  foreach(Candidate IN LISTS ARGN)
    if(TARGET "${Candidate}")
      target_link_libraries("${Target}" "${Visibility}" "${Candidate}")
      return()
    endif()
  endforeach()
  message(FATAL_ERROR "None of the expected dependency targets exists: ${ARGN}")
endfunction()

broccoli_link_first_available(BroccoliEngine PRIVATE cpr::cpr)
broccoli_link_first_available(BroccoliEngine PRIVATE httplib::httplib)
broccoli_link_first_available(BroccoliEngine PRIVATE unofficial::enet::enet enet)
broccoli_link_first_available(BroccoliEngine PUBLIC imgui::imgui imgui)
broccoli_link_first_available(BroccoliEngine PRIVATE raylib raylib_static)
broccoli_link_first_available(BroccoliEngine PRIVATE glfw)
broccoli_link_first_available(BroccoliEngine PRIVATE
  unofficial::minizip::minizip minizip::minizip MINIZIP::minizip)

if(WIN32)
  target_link_libraries(BroccoliEngine PRIVATE
    ws2_32 winmm crypt32 bcrypt secur32 iphlpapi shell32
  )

  set(BROCCOLI_EOS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/EOS/SDK")
  set(BROCCOLI_EOS_LIBRARY "${BROCCOLI_EOS_ROOT}/Lib/EOSSDK-Win64-Shipping.lib")
  set(BROCCOLI_EOS_DLL "${BROCCOLI_EOS_ROOT}/Bin/EOSSDK-Win64-Shipping.dll")

  if(EXISTS "${BROCCOLI_EOS_LIBRARY}" AND EXISTS "${BROCCOLI_EOS_DLL}")
    add_library(BroccoliEOS SHARED IMPORTED GLOBAL)
    set_target_properties(BroccoliEOS PROPERTIES
      IMPORTED_IMPLIB "${BROCCOLI_EOS_LIBRARY}"
      IMPORTED_LOCATION "${BROCCOLI_EOS_DLL}"
      INTERFACE_INCLUDE_DIRECTORIES "${BROCCOLI_EOS_ROOT}/Include"
    )
    target_link_libraries(BroccoliEngine PRIVATE BroccoliEOS)
  else()
    set(MISSING_EOS_DETAILS "")
    if(NOT EXISTS "${BROCCOLI_EOS_LIBRARY}")
      string(APPEND MISSING_EOS_DETAILS "  Library not found: ${BROCCOLI_EOS_LIBRARY}\n")
    endif()
    if(NOT EXISTS "${BROCCOLI_EOS_DLL}")
      string(APPEND MISSING_EOS_DETAILS "  DLL not found: ${BROCCOLI_EOS_DLL}\n")
    endif()
    message(FATAL_ERROR "EOS SDK components are missing:\n${MISSING_EOS_DETAILS}")
  endif()
endif()

set_target_properties(BroccoliEngine PROPERTIES
  OUTPUT_NAME BroccoliEngine
  RUNTIME_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  PDB_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
)

set(BROCCOLI_PLUGIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BroccoliPlugin.cmake")
foreach(PluginName IN LISTS BROCCOLI_PLUGINS)
  set(PluginBuildFile "${BROCCOLI_PLUGIN_ROOT}/${PluginName}/Build.cmake")
  if(NOT EXISTS "${PluginBuildFile}")
    message(FATAL_ERROR "Plugin '${PluginName}' does not contain Build.cmake: ${PluginBuildFile}")
  endif()
  include("${PluginBuildFile}")
  if(NOT TARGET "${PluginName}")
    message(FATAL_ERROR "Plugin '${PluginName}' Build.cmake did not define its target.")
  endif()
endforeach()

if(WIN32)
  add_executable(BroccoliBootstrap WIN32
    "${CMAKE_CURRENT_SOURCE_DIR}/Bootstrap/Bootstrap.cpp"
  )
  target_compile_features(BroccoliBootstrap PRIVATE cxx_std_20)
  target_link_libraries(BroccoliBootstrap PRIVATE user32 shlwapi shell32)
  set_target_properties(BroccoliBootstrap PROPERTIES
    OUTPUT_NAME BroccoliBootstrap
    RUNTIME_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
    PDB_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  )
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${BROCCOLI_ENGINE_FILES}
)
