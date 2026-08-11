include_guard(GLOBAL)

include(CMakeParseArguments)

function(broccoli_add_game)
  set(Options)
  set(OneValueArguments NAME)
  cmake_parse_arguments(BroccoliGame "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(BroccoliGame_NAME)
    set(GameName "${BroccoliGame_NAME}")
  elseif(DEFINED BROCCOLI_PROJECT_NAME)
    set(GameName "${BROCCOLI_PROJECT_NAME}")
  else()
    message(FATAL_ERROR "broccoli_add_game requires NAME or BROCCOLI_PROJECT_NAME.")
  endif()

  if(NOT TARGET Broccoli::Engine)
    message(FATAL_ERROR "Game requires the Broccoli::Engine target")
  endif()

  foreach(RequiredVariable BROCCOLI_ENGINE_ROOT BROCCOLI_GAME_ROOT BROCCOLI_OUTPUT_ROOT)
    if(NOT DEFINED ${RequiredVariable})
      message(FATAL_ERROR "${RequiredVariable} must be defined before calling broccoli_add_game.")
    endif()
  endforeach()

  file(GLOB_RECURSE BroccoliGameFiles
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp"
  )

  add_executable(${GameName} WIN32 ${BroccoliGameFiles})
  target_compile_features(${GameName} PRIVATE cxx_std_20)
  target_compile_definitions(${GameName} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Editor>:_EDITOR>
    $<$<CONFIG:Release>:_RELEASE>
    BROCCOLI_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    BROCCOLI_GAME_NAME="${GameName}"
  )
  target_include_directories(${GameName} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
  target_link_libraries(${GameName} PRIVATE Broccoli::Engine)
  add_dependencies(${GameName} BroccoliBootstrap)

  if(MSVC)
    broccoli_apply_common_msvc_options(${GameName})
    target_link_libraries(${GameName} PRIVATE winmm)
    set_property(TARGET ${GameName} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
  endif()

  set_target_properties(${GameName} PROPERTIES
    OUTPUT_NAME "${GameName}-game"
    RUNTIME_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
    PDB_OUTPUT_DIRECTORY "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>"
  )

  find_program(BROCCOLI_UV_EXECUTABLE uv REQUIRED)
  set(BroccoliEngineSourceDir "${BROCCOLI_ENGINE_ROOT}/Engine")
  set(BroccoliBuildToolsDir "${BROCCOLI_ENGINE_ROOT}/Tools/Build")
  set(BroccoliConvertLevelsScript "${BROCCOLI_ENGINE_ROOT}/Tools/ConvertLevels.py")
  set(BroccoliEosBinary "${BROCCOLI_ENGINE_ROOT}/Engine/ThirdParty/EOS/SDK/Bin/EOSSDK-Win64-Shipping.dll")

  add_custom_command(TARGET ${GameName} PRE_BUILD
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDir}" --frozen
            python -m broccoli_build prepare-output
            --output-dir "$<TARGET_FILE_DIR:${GameName}>"
    VERBATIM
  )

  add_custom_command(TARGET ${GameName} POST_BUILD
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDir}" --frozen
            python -m broccoli_build stage-runtime
            --configuration "$<CONFIG>"
            --engine-dir "${BroccoliEngineSourceDir}"
            --game-dir "${BROCCOLI_GAME_ROOT}"
            --output-dir "$<TARGET_FILE_DIR:${GameName}>"
            --engine-binary "$<TARGET_FILE:BroccoliEngine>"
            --game-name "${GameName}"
            --eos-binary "${BroccoliEosBinary}"
            --convert-levels-script "${BroccoliConvertLevelsScript}"
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDir}" --frozen
            python -m broccoli_build verify-runtime
            --configuration "$<CONFIG>"
            --output-dir "$<TARGET_FILE_DIR:${GameName}>"
            --game-name "${GameName}"
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDir}" --frozen
            python -m broccoli_build package-runtime
            --configuration "$<CONFIG>"
            --output-dir "$<TARGET_FILE_DIR:${GameName}>"
            --publish-dir "${BROCCOLI_OUTPUT_ROOT}/Publish/$<CONFIG>"
            --game-binary "$<TARGET_FILE:${GameName}>"
            --engine-binary "$<TARGET_FILE:BroccoliEngine>"
            --game-name "${GameName}"
            --eos-binary "${BroccoliEosBinary}"
            --online-resources-dir "$<TARGET_FILE_DIR:${GameName}>/Resources-EOS"
            --convert-levels-script "${BroccoliConvertLevelsScript}"
            --bootstrap-binary "$<TARGET_FILE:BroccoliBootstrap>"
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDir}" --frozen
            python -m broccoli_build verify-runtime
            --configuration "$<CONFIG>"
            --output-dir "$<TARGET_FILE_DIR:${GameName}>"
            --game-name "${GameName}"
            --publish-dir "${BROCCOLI_OUTPUT_ROOT}/Publish/$<CONFIG>"
    VERBATIM
  )

  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${BroccoliGameFiles})
endfunction()
