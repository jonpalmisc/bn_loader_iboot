cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(view_iboot)

option(BN_API_PATH "Path to the Binary Ninja API")
if(NOT BN_API_PATH)
	message(FATAL_ERROR "Project option `BN_API_PATH` must be set.")
endif()

# The UI API is not needed for this plugin, so we can skip compiling some parts
# of the API.
set(HEADLESS ON)

add_subdirectory(${BN_API_PATH} ${PROJECT_BINARY_DIR}/api)

set(PLUGIN_SOURCE
	plugin.cpp
	securebootview.h
	securebootview.cpp
	viewsupport.h
	viewsupport.cpp)

add_library(view_iboot SHARED ${PLUGIN_SOURCE})
target_compile_features(view_iboot PRIVATE cxx_std_17 c_std_99)
target_link_libraries(view_iboot PRIVATE binaryninjaapi)

# Plugins need to compile with `-fPIC` on Linux.
if(${CMAKE_SYSTEM_NAME} STREQUAL "LINUX")
	target_compile_options(view_iboot PRIVATE "-fPIC")
endif()

bn_install_plugin(view_iboot)
