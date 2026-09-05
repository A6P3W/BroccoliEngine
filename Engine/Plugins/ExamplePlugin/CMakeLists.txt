set(BROCCOLI_PLUGIN_OUTPUT_DIRECTORY
  "${BROCCOLI_OUTPUT_ROOT}/Bin/x64/$<CONFIG>/Plugins/ExamplePlugin")

add_library(ExamplePlugin SHARED
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/ExamplePlugin.cpp"
)
target_compile_features(ExamplePlugin PRIVATE cxx_std_20)
target_link_libraries(ExamplePlugin PRIVATE Broccoli::Engine)
if(MSVC)
  broccoli_apply_common_msvc_options(ExamplePlugin)
  set_property(TARGET ExamplePlugin PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:Editor>>:Debug>DLL")
endif()
set_target_properties(ExamplePlugin PROPERTIES
  OUTPUT_NAME ExamplePlugin
  RUNTIME_OUTPUT_DIRECTORY "${BROCCOLI_PLUGIN_OUTPUT_DIRECTORY}"
  LIBRARY_OUTPUT_DIRECTORY "${BROCCOLI_PLUGIN_OUTPUT_DIRECTORY}"
  ARCHIVE_OUTPUT_DIRECTORY "${BROCCOLI_PLUGIN_OUTPUT_DIRECTORY}"
  PDB_OUTPUT_DIRECTORY "${BROCCOLI_PLUGIN_OUTPUT_DIRECTORY}"
)
add_custom_command(TARGET ExamplePlugin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
          "$<TARGET_FILE_DIR:ExamplePlugin>/plugin.json"
  VERBATIM
)
