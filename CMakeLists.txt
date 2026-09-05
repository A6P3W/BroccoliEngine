cmake_minimum_required(VERSION 4.2)

set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine" CACHE PATH
    "Directory containing the BroccoliEngine vcpkg manifest")

project(BroccoliEngineWorkspace LANGUAGES CXX)

set(BROCCOLI_PROJECT_NAME "Launcher")
set(BROCCOLI_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BROCCOLI_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BROCCOLI_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Launcher")
set(BROCCOLI_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/ProjectDefaults.cmake")
include("${BROCCOLI_ENGINE_ROOT}/Engine/cmake/BroccoliGameTarget.cmake")
include("${BROCCOLI_PROJECT_ROOT}/Intermediate/Generated/Plugins.cmake")

add_subdirectory(Engine)
add_subdirectory(Launcher)

foreach(Configuration IN ITEMS Debug Editor Release)
  add_custom_target("BroccoliProjectBuild_${Configuration}")
  add_dependencies("BroccoliProjectBuild_${Configuration}" "${BROCCOLI_PROJECT_NAME}")
  broccoli_add_configured_plugins_to_target(
    TARGET "BroccoliProjectBuild_${Configuration}"
    CONFIGURATION "${Configuration}"
  )
endforeach()
