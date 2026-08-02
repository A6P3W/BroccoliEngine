cmake_minimum_required(VERSION 3.25)

# vcpkg manifest is owned by the reusable Engine project.
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Engine" CACHE PATH
    "Directory containing the BroccoliEngine vcpkg manifest")

project(BroccoliEngineWorkspace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Editor;Release" CACHE STRING "" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Editor Release)
endif()

add_subdirectory(Engine)
add_subdirectory(Launcher)
