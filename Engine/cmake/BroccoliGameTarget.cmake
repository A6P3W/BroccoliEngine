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
  target_compile_features(${GameName} PRIVATE cxx_std_26)
  broccoli_enable_reflection(${GameName})
  broccoli_apply_gnu_debug_options(${GameName})
  target_compile_definitions(${GameName} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Editor>:_EDITOR>
    $<$<CONFIG:Release>:_RELEASE>
    BROCCOLI_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    BROCCOLI_GAME_NAME="${GameName}"
  )
  target_include_directories(${GameName} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
  target_link_libraries(${GameName} PRIVATE Broccoli::Engine)
  broccoli_link_configured_plugins_to_game(TARGET ${GameName})
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

  set(BroccoliEngineSourceDir "${BROCCOLI_ENGINE_ROOT}/Engine")
  set(RuntimeDependencies)
  if(MINGW)
    get_filename_component(CompilerDirectory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(RuntimeName IN ITEMS libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
      list(APPEND RuntimeDependencies "${CompilerDirectory}/${RuntimeName}")
    endforeach()
  endif()
  foreach(Configuration IN ITEMS Debug Editor Release)
    string(TOUPPER "${Configuration}" ConfigurationUpper)
    set(PluginDependencies)
    foreach(Plugin IN LISTS BROCCOLI_PLUGINS_${ConfigurationUpper})
      get_target_property(Dependencies "${Plugin}" BROCCOLI_RUNTIME_DEPENDENCIES)
      if(Dependencies)
        list(APPEND PluginDependencies ${Dependencies})
      endif()
    endforeach()
    file(GENERATE OUTPUT "${BROCCOLI_OUTPUT_ROOT}/Intermediate/Generated/runtime-${Configuration}.json"
      CONTENT "{\"engine_dir\":\"${BroccoliEngineSourceDir}\",\"game_dir\":\"${BROCCOLI_GAME_ROOT}\",\"game_name\":\"${GameName}\",\"runtime\":[\"$<JOIN:${RuntimeDependencies},\",\">\"],\"plugin_dependencies\":[\"$<JOIN:${PluginDependencies},\",\">\"]}\n"
      CONDITION "$<CONFIG:${Configuration}>")
  endforeach()
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${BroccoliGameFiles})
endfunction()
