cmake_minimum_required(VERSION 3.25)

# A game repository may include this directory after adding Engine separately.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  if(NOT TARGET Broccoli::Engine)
    set(BROCCOLI_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Engine"
        CACHE PATH "Path to the BroccoliEngine source tree")
    set(VCPKG_MANIFEST_DIR "${BROCCOLI_ENGINE_SOURCE_DIR}" CACHE PATH
        "Directory containing the BroccoliEngine vcpkg manifest")
  endif()

  project(BroccoliLauncher LANGUAGES CXX)
  include("${CMAKE_CURRENT_LIST_DIR}/../ProjectDefaults.cmake")

  if(NOT TARGET Broccoli::Engine)
    add_subdirectory("${BROCCOLI_ENGINE_SOURCE_DIR}"
                     "${CMAKE_CURRENT_BINARY_DIR}/BroccoliEngine")
  endif()
endif()

if(NOT TARGET Broccoli::Engine)
  message(FATAL_ERROR "Launcher requires the Broccoli::Engine target")
endif()

file(GLOB_RECURSE BROCCOLI_LAUNCHER_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp"
)

add_executable(Launcher WIN32 ${BROCCOLI_LAUNCHER_FILES})

target_compile_features(Launcher PRIVATE cxx_std_20)
target_compile_definitions(Launcher PRIVATE
  _CONSOLE
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Editor>:_EDITOR>
  $<$<CONFIG:Release>:_RELEASE>
)
target_include_directories(Launcher PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source
  ${CMAKE_CURRENT_SOURCE_DIR}/Game
  ${CMAKE_CURRENT_SOURCE_DIR}/Config
)
target_link_libraries(Launcher PRIVATE Broccoli::Engine)

if(MSVC)
  broccoli_apply_common_msvc_options(Launcher)
  target_link_options(Launcher PRIVATE
    $<$<CONFIG:Debug>:/DELAYLOAD:BroccoliEngine.dll>
  )
  target_link_libraries(Launcher PRIVATE winmm $<$<CONFIG:Debug>:delayimp>)
  set_property(TARGET Launcher PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
endif()

set_target_properties(Launcher PROPERTIES
  OUTPUT_NAME Launcher-game
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Bin/x64/$<CONFIG>"
  PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Bin/x64/$<CONFIG>"
)

add_custom_command(TARGET Launcher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:BroccoliEngine> $<TARGET_FILE_DIR:Launcher>
  VERBATIM
)

if(TARGET BroccoliEOS)
  add_custom_command(TARGET Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:BroccoliEOS> $<TARGET_FILE_DIR:Launcher>
    VERBATIM
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
  add_custom_command(TARGET Launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
            "$<TARGET_FILE_DIR:Launcher>/Resources"
    VERBATIM
  )
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${BROCCOLI_LAUNCHER_FILES}
)
