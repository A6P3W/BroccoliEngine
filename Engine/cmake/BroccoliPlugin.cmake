include_guard(GLOBAL)

include(CMakeParseArguments)

function(broccoli_configure_plugin)
  set(Options)
  set(OneValueArguments TARGET)
  cmake_parse_arguments(BroccoliPlugin "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(NOT BroccoliPlugin_TARGET)
    message(FATAL_ERROR "broccoli_configure_plugin requires TARGET.")
  endif()

  set_property(TARGET "${BroccoliPlugin_TARGET}" PROPERTY EXCLUDE_FROM_ALL TRUE)
  foreach(Configuration IN ITEMS Debug Editor Release)
    string(TOUPPER "${Configuration}" ConfigurationUpper)
    set(PluginListVariable "BROCCOLI_PLUGINS_${ConfigurationUpper}")
    if(NOT "${BroccoliPlugin_TARGET}" IN_LIST ${PluginListVariable})
      set_property(
        TARGET "${BroccoliPlugin_TARGET}"
        PROPERTY "EXCLUDE_FROM_DEFAULT_BUILD_${Configuration}" TRUE
      )
    endif()
  endforeach()
endfunction()

function(broccoli_add_configured_plugins_to_target)
  set(Options)
  set(OneValueArguments TARGET CONFIGURATION)
  cmake_parse_arguments(BroccoliPluginBuild "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(NOT BroccoliPluginBuild_TARGET OR NOT BroccoliPluginBuild_CONFIGURATION)
    message(FATAL_ERROR "broccoli_add_configured_plugins_to_target requires TARGET and CONFIGURATION.")
  endif()

  string(TOUPPER "${BroccoliPluginBuild_CONFIGURATION}" ConfigurationUpper)
  set(PluginListVariable "BROCCOLI_PLUGINS_${ConfigurationUpper}")
  foreach(Plugin IN LISTS ${PluginListVariable})
    if(TARGET "${Plugin}")
      add_dependencies("${BroccoliPluginBuild_TARGET}" "${Plugin}")
    endif()
  endforeach()
endfunction()

function(broccoli_add_plugin)
  set(Options)
  set(OneValueArguments NAME)
  cmake_parse_arguments(BroccoliPlugin "${Options}" "${OneValueArguments}" "" ${ARGN})

  if(NOT BroccoliPlugin_NAME)
    message(FATAL_ERROR "broccoli_add_plugin requires NAME.")
  endif()
  if(TARGET "${BroccoliPlugin_NAME}")
    message(FATAL_ERROR "Plugin target '${BroccoliPlugin_NAME}' already exists.")
  endif()
  if(NOT DEFINED BROCCOLI_PLUGIN_ROOT)
    message(FATAL_ERROR "BROCCOLI_PLUGIN_ROOT must be defined before adding a plugin.")
  endif()

  set(PluginDirectory "${BROCCOLI_PLUGIN_ROOT}/${BroccoliPlugin_NAME}")
  if(NOT IS_DIRECTORY "${PluginDirectory}")
    message(FATAL_ERROR "Plugin '${BroccoliPlugin_NAME}' directory does not exist: ${PluginDirectory}")
  endif()

  set(PluginSourceDirectory "${PluginDirectory}/Source")
  if(NOT IS_DIRECTORY "${PluginSourceDirectory}")
    message(FATAL_ERROR "Plugin '${BroccoliPlugin_NAME}' Source directory does not exist: ${PluginSourceDirectory}")
  endif()

  set(PluginManifest "${PluginDirectory}/plugin.json")
  if(NOT EXISTS "${PluginManifest}")
    message(FATAL_ERROR "Plugin '${BroccoliPlugin_NAME}' plugin.json does not exist: ${PluginManifest}")
  endif()

  file(GLOB_RECURSE PluginSources
    CONFIGURE_DEPENDS
    "${PluginSourceDirectory}/*.cpp"
    "${PluginSourceDirectory}/*.c"
    "${PluginSourceDirectory}/*.h"
    "${PluginSourceDirectory}/*.hpp"
    "${PluginSourceDirectory}/*.inl"
  )
  if(NOT PluginSources)
    message(FATAL_ERROR "Plugin '${BroccoliPlugin_NAME}' does not contain source files: ${PluginSourceDirectory}")
  endif()

  set(PluginOutputDirectory
    "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>/Plugins/${BroccoliPlugin_NAME}")
  add_library("${BroccoliPlugin_NAME}" SHARED ${PluginSources})
  target_compile_features("${BroccoliPlugin_NAME}" PRIVATE cxx_std_20)
  target_link_libraries("${BroccoliPlugin_NAME}" PRIVATE Broccoli::Engine)
  if(MSVC)
    broccoli_apply_common_msvc_options("${BroccoliPlugin_NAME}")
    set_property(TARGET "${BroccoliPlugin_NAME}" PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
  endif()
  set_target_properties("${BroccoliPlugin_NAME}" PROPERTIES
    OUTPUT_NAME "${BroccoliPlugin_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${PluginOutputDirectory}"
    LIBRARY_OUTPUT_DIRECTORY "${PluginOutputDirectory}"
    ARCHIVE_OUTPUT_DIRECTORY "${PluginOutputDirectory}"
    PDB_OUTPUT_DIRECTORY "${PluginOutputDirectory}"
  )
  broccoli_configure_plugin(TARGET "${BroccoliPlugin_NAME}")
  add_custom_command(TARGET "${BroccoliPlugin_NAME}" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PluginManifest}"
            "$<TARGET_FILE_DIR:${BroccoliPlugin_NAME}>/plugin.json"
    VERBATIM
  )
  source_group(TREE "${PluginSourceDirectory}" FILES ${PluginSources})
endfunction()
