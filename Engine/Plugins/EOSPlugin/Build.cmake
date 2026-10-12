if(NOT MINGW)
  message(FATAL_ERROR "EOSPlugin requires MinGW-w64 GCC.")
endif()
broccoli_add_plugin(NAME EOSPlugin)
set(EOS_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/ThirdParty/EOS/SDK")
if(NOT EXISTS "${EOS_SDK_ROOT}/Include/eos_sdk.h" OR
   NOT EXISTS "${EOS_SDK_ROOT}/Lib/EOSSDK-Win64-Shipping.lib" OR
   NOT EXISTS "${EOS_SDK_ROOT}/Bin/EOSSDK-Win64-Shipping.dll")
  message(FATAL_ERROR "EOSPlugin requires the EOS SDK at ${EOS_SDK_ROOT}")
endif()
add_library(EOSPluginSDK SHARED IMPORTED GLOBAL)
set_target_properties(EOSPluginSDK PROPERTIES
  IMPORTED_IMPLIB "${EOS_SDK_ROOT}/Lib/EOSSDK-Win64-Shipping.lib"
  IMPORTED_LOCATION "${EOS_SDK_ROOT}/Bin/EOSSDK-Win64-Shipping.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${EOS_SDK_ROOT}/Include")
target_include_directories(EOSPlugin
  PUBLIC "${CMAKE_CURRENT_LIST_DIR}/Source/Public"
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Source/Private" "${BROCCOLI_ENGINE_ROOT}/PrivateConfig")
target_compile_definitions(EOSPlugin PRIVATE EOS_PLUGIN_BUILD)
target_link_libraries(EOSPlugin PRIVATE EOSPluginSDK)
broccoli_link_first_available(EOSPlugin PRIVATE
  unofficial::minizip::minizip minizip::minizip MINIZIP::minizip)
set_property(TARGET EOSPlugin PROPERTY BROCCOLI_RUNTIME_DEPENDENCIES
  "${EOS_SDK_ROOT}/Bin/EOSSDK-Win64-Shipping.dll")
