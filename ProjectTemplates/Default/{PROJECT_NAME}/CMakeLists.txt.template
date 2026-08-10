cmake_minimum_required(VERSION 4.2)

if(NOT TARGET Broccoli::Engine)
  message(FATAL_ERROR "Game requires the Broccoli::Engine target")
endif()

file(GLOB_RECURSE BROCCOLI_GAME_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp"
)

add_executable(${BROCCOLI_PROJECT_NAME} WIN32 ${BROCCOLI_GAME_FILES})

target_compile_features(${BROCCOLI_PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${BROCCOLI_PROJECT_NAME} PRIVATE
  _CONSOLE
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Editor>:_EDITOR>
  $<$<CONFIG:Release>:_RELEASE>
  BROCCOLI_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
  BROCCOLI_GAME_NAME="${BROCCOLI_PROJECT_NAME}"
)
target_include_directories(${BROCCOLI_PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)
target_link_libraries(${BROCCOLI_PROJECT_NAME} PRIVATE Broccoli::Engine)
add_dependencies(${BROCCOLI_PROJECT_NAME} BroccoliBootstrap)

if(MSVC)
  broccoli_apply_common_msvc_options(${BROCCOLI_PROJECT_NAME})
  target_link_libraries(${BROCCOLI_PROJECT_NAME} PRIVATE winmm)
  set_property(TARGET ${BROCCOLI_PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
endif()

set_target_properties(${BROCCOLI_PROJECT_NAME} PROPERTIES
  OUTPUT_NAME "${BROCCOLI_PROJECT_NAME}-game"
  RUNTIME_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  PDB_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
)

find_program(BROCCOLI_UV_EXECUTABLE uv REQUIRED)
set(BROCCOLI_ENGINE_SOURCE_DIR "${BROCCOLI_ENGINE_ROOT}/Engine")
set(BROCCOLI_BUILD_TOOLS_DIR "${BROCCOLI_ENGINE_ROOT}/Tools/Build")
set(BROCCOLI_CONVERT_LEVELS_SCRIPT "${BROCCOLI_ENGINE_ROOT}/Tools/ConvertLevels.py")
set(BROCCOLI_EOS_BINARY
    "${BROCCOLI_ENGINE_ROOT}/Engine/ThirdParty/EOS/SDK/Bin/EOSSDK-Win64-Shipping.dll")

add_custom_command(TARGET ${BROCCOLI_PROJECT_NAME} PRE_BUILD
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen
          python -m broccoli_build prepare-output
          --output-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>"
  VERBATIM
)

add_custom_command(TARGET ${BROCCOLI_PROJECT_NAME} POST_BUILD
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen
          python -m broccoli_build stage-runtime
          --configuration "$<CONFIG>"
          --engine-dir "${BROCCOLI_ENGINE_SOURCE_DIR}"
          --game-dir "${BROCCOLI_GAME_ROOT}"
          --output-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>"
          --engine-binary "$<TARGET_FILE:BroccoliEngine>"
          --game-name "${BROCCOLI_PROJECT_NAME}"
          --eos-binary "${BROCCOLI_EOS_BINARY}"
          --convert-levels-script "${BROCCOLI_CONVERT_LEVELS_SCRIPT}"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen
          python -m broccoli_build verify-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>"
          --game-name "${BROCCOLI_PROJECT_NAME}"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen
          python -m broccoli_build package-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>"
          --publish-dir "${BROCCOLI_OUTPUT_ROOT}/Publish/$<CONFIG>"
          --game-binary "$<TARGET_FILE:${BROCCOLI_PROJECT_NAME}>"
          --engine-binary "$<TARGET_FILE:BroccoliEngine>"
          --game-name "${BROCCOLI_PROJECT_NAME}"
          --eos-binary "${BROCCOLI_EOS_BINARY}"
          --online-resources-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>/Resources-EOS"
          --convert-levels-script "${BROCCOLI_CONVERT_LEVELS_SCRIPT}"
          --bootstrap-binary "$<TARGET_FILE:BroccoliBootstrap>"
  COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BROCCOLI_BUILD_TOOLS_DIR}" --frozen
          python -m broccoli_build verify-runtime
          --configuration "$<CONFIG>"
          --output-dir "$<TARGET_FILE_DIR:${BROCCOLI_PROJECT_NAME}>"
          --game-name "${BROCCOLI_PROJECT_NAME}"
          --publish-dir "${BROCCOLI_OUTPUT_ROOT}/Publish/$<CONFIG>"
  VERBATIM
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
  ${BROCCOLI_GAME_FILES}
)
