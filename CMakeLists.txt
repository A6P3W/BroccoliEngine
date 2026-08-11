cmake_minimum_required(VERSION 4.2)

set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine" CACHE PATH
    "Directory containing the BroccoliEngine vcpkg manifest")

project(BroccoliEngineWorkspace LANGUAGES CXX)

set(BROCCOLI_PROJECT_NAME "Launcher")
set(BROCCOLI_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BROCCOLI_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Launcher")
set(BROCCOLI_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/ProjectDefaults.cmake")
include("${BROCCOLI_ENGINE_ROOT}/Engine/cmake/BroccoliGameTarget.cmake")

add_subdirectory(Engine)
add_subdirectory(Launcher)
