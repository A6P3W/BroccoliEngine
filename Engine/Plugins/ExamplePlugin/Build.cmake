broccoli_add_plugin(
  NAME ExamplePlugin
)

target_include_directories(ExamplePlugin
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/Source/Public"
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Source/Private"
)
target_compile_definitions(ExamplePlugin PRIVATE EXAMPLE_PLUGIN_BUILD)
