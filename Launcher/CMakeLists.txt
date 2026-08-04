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
  $<$<CONFIG:Editor>:BROCCOLI_GAME_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/">
  BROCCOLI_GAME_NAME="Launcher"
)
target_include_directories(Launcher PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source
  ${CMAKE_CURRENT_SOURCE_DIR}/Game
  ${CMAKE_CURRENT_SOURCE_DIR}/Config
)
target_link_libraries(Launcher PRIVATE Broccoli::Engine)
add_dependencies(Launcher BroccoliBootstrap)

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
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Launcher>"
)

find_program(BROCCOLI_UV_EXECUTABLE uv REQUIRED)
set(BROCCOLI_ENGINE_SOURCE_DIR_RESOLVED "${CMAKE_CURRENT_SOURCE_DIR}/../Engine")
if(DEFINED BROCCOLI_ENGINE_SOURCE_DIR)
  set(BROCCOLI_ENGINE_SOURCE_DIR_RESOLVED "${BROCCOLI_ENGINE_SOURCE_DIR}")
endif()
set(BROCCOLI_BUILD_TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/Build")
set(BROCCOLI_CONVERT_LEVELS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/ConvertLevels.py")
set(BROCCOLI_EOS_BINARY "${CMAKE_CURRENT_SOURCE_DIR}/../Engine/ThirdParty/EOS/SDK/Bin/EOSSDK-Win64-Shipping.dll")

add_custom_command(TARGET Launcher PRE_BUILD
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen python -m broccoli_build prepare-output
          --output-dir "$<TARGET_FILE_DIR:Launcher>"
  VERBATIM
)

add_custom_command(TARGET Launcher POST_BUILD
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen python -m broccoli_build stage-runtime
          --configuration "$<CONFIG>"
          --engine-dir "${BROCCOLI_ENGINE_SOURCE_DIR_RESOLVED}"
          --game-dir "${CMAKE_CURRENT_SOURCE_DIR}"
          --output-dir "$<TARGET_FILE_DIR:Launcher>"
          --engine-binary "$<TARGET_FILE:BroccoliEngine>"
          --game-name "Launcher"
          --eos-binary "${BROCCOLI_EOS_BINARY}"
          --convert-levels-script "${BROCCOLI_CONVERT_LEVELS_SCRIPT}"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen python -m broccoli_build verify-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:Launcher>"
          --game-name "Launcher"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen python -m broccoli_build package-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:Launcher>"
          --publish-dir "${CMAKE_CURRENT_SOURCE_DIR}/../Publish/$<CONFIG>"
          --game-binary "$<TARGET_FILE:Launcher>"
          --engine-binary "$<TARGET_FILE:BroccoliEngine>"
          --game-name "Launcher"
          --eos-binary "${BROCCOLI_EOS_BINARY}"
          --online-resources-dir "$<TARGET_FILE_DIR:Launcher>/Resources-EOS"
          --convert-levels-script "${BROCCOLI_CONVERT_LEVELS_SCRIPT}"
          --bootstrap-binary "$<TARGET_FILE:BroccoliBootstrap>"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen python -m broccoli_build verify-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:Launcher>"
          --game-name "Launcher"
          --publish-dir "${CMAKE_CURRENT_SOURCE_DIR}/../Publish/$<CONFIG>"
  VERBATIM
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${BROCCOLI_LAUNCHER_FILES}
)