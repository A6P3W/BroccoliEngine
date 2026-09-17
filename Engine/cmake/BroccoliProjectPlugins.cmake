include_guard(GLOBAL)

function(broccoli_configure_project_plugins)
  foreach(RequiredVariable BROCCOLI_PROJECT_ROOT BROCCOLI_ENGINE_ROOT)
    if(NOT DEFINED ${RequiredVariable})
      message(FATAL_ERROR "${RequiredVariable} must be defined before configuring project plugins.")
    endif()
  endforeach()

  set(ProjectSettingsFile "${BROCCOLI_PROJECT_ROOT}/.broccoli-project.json")
  if(NOT EXISTS "${ProjectSettingsFile}")
    message(FATAL_ERROR "Project settings do not exist: ${ProjectSettingsFile}")
  endif()

  set(BroccoliBuildToolsDirectory "${BROCCOLI_ENGINE_ROOT}/Tools/Build")
  file(GLOB_RECURSE BroccoliBuildToolSources CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${BroccoliBuildToolsDirectory}/broccoli_build/*.py"
  )
  list(APPEND BroccoliBuildToolSources
    "${BroccoliBuildToolsDirectory}/pyproject.toml"
    "${BroccoliBuildToolsDirectory}/uv.lock"
  )

  set_property(
    DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ProjectSettingsFile}"
    ${BroccoliBuildToolSources}
  )

  find_program(BROCCOLI_UV_EXECUTABLE NAMES uv REQUIRED)
  execute_process(
    COMMAND "${BROCCOLI_UV_EXECUTABLE}" run --project "${BroccoliBuildToolsDirectory}" --frozen
            python -m broccoli_build generate-plugins --project-dir "${BROCCOLI_PROJECT_ROOT}"
    RESULT_VARIABLE GeneratePluginsResult
    OUTPUT_VARIABLE GeneratePluginsOutput
    ERROR_VARIABLE GeneratePluginsError
  )
  if(NOT GeneratePluginsResult EQUAL 0)
    message(FATAL_ERROR
      "Failed to generate project plugin configuration (exit code ${GeneratePluginsResult}).\n"
      "${GeneratePluginsOutput}${GeneratePluginsError}"
    )
  endif()

  set(GeneratedPluginsFile "${BROCCOLI_PROJECT_ROOT}/Intermediate/Generated/Plugins.cmake")
  if(NOT EXISTS "${GeneratedPluginsFile}")
    message(FATAL_ERROR "Plugin configuration was not generated: ${GeneratedPluginsFile}")
  endif()
  include("${GeneratedPluginsFile}")

  foreach(PluginVariable IN ITEMS
      BROCCOLI_PLUGINS
      BROCCOLI_PLUGINS_DEBUG
      BROCCOLI_PLUGINS_EDITOR
      BROCCOLI_PLUGINS_RELEASE)
    set("${PluginVariable}" "${${PluginVariable}}" PARENT_SCOPE)
  endforeach()
endfunction()
