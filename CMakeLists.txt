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
include("${BROCCOLI_ENGINE_ROOT}/Engine/cmake/BroccoliProjectPlugins.cmake")
broccoli_configure_project_plugins()

add_subdirectory(Engine)
add_subdirectory(Launcher)

include(CTest)
if(BUILD_TESTING)
  add_executable(BroccoliReflectionTests Tests/Reflection/ReflectionTests.cpp)
  target_link_libraries(BroccoliReflectionTests PRIVATE BroccoliEngine)
  target_compile_features(BroccoliReflectionTests PRIVATE cxx_std_26)
  target_compile_options(BroccoliReflectionTests PRIVATE -freflection)
  add_test(NAME BroccoliReflectionTests COMMAND BroccoliReflectionTests)
  set_tests_properties(BroccoliReflectionTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:BroccoliEngine>")
endif()

foreach(Configuration IN ITEMS Debug Editor Release)
  add_custom_target("BroccoliProjectBuild_${Configuration}")
  add_dependencies("BroccoliProjectBuild_${Configuration}" "${BROCCOLI_PROJECT_NAME}")
  if(BUILD_TESTING AND Configuration STREQUAL "Debug")
    add_dependencies("BroccoliProjectBuild_${Configuration}" BroccoliReflectionTests)
  endif()
  broccoli_add_configured_plugins_to_target(
    TARGET "BroccoliProjectBuild_${Configuration}"
    CONFIGURATION "${Configuration}"
  )
endforeach()
