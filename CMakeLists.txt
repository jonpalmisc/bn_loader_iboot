cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(loader_iboot)

set(PLUGIN_SOURCE
	src/securebootview.h
	src/securebootview.cpp
	src/plugin.cpp)

add_library(loader_iboot SHARED ${PLUGIN_SOURCE})
target_compile_features(loader_iboot PRIVATE cxx_std_17 c_std_99)
target_link_libraries(loader_iboot PRIVATE binaryninjaapi)

# Plugins need to compile with `-fPIC` on Linux.
if(${CMAKE_SYSTEM_NAME} STREQUAL "LINUX")
	target_compile_options(loader_iboot PRIVATE "-fPIC")
endif()

# Add vendored API dependency if enabled.
option(BN_VENDOR_API "Use the Binary Ninja API submodule" ON)
if(BN_VENDOR_API)
	set(HEADLESS ON)
	add_subdirectory(vendor/api)

	# Auto-configure an `install` target.
	bn_install_plugin(loader_iboot)
endif()

