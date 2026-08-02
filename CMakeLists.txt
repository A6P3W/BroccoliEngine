cmake_minimum_required(VERSION 3.25)

# vcpkg manifest is owned by the reusable Engine project.
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine" CACHE PATH
    "Directory containing the BroccoliEngine vcpkg manifest")

project(BroccoliEngineWorkspace LANGUAGES CXX)

include("${CMAKE_CURRENT_LIST_DIR}/ProjectDefaults.cmake")

add_subdirectory(Engine)
add_subdirectory(Launcher)
