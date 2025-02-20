cmake_minimum_required(VERSION 3.10)
project(metricz-cpp VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_EMBEDDED "Enable embedded system optimizations" OFF)
option(ENABLE_HEADER_ONLY "Enable header-only mode" OFF)

# Dependencies
include(FetchContent)
if(NOT HTTPLIB_FOUND)
	FetchContent_Declare(
			httplib
			GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
			GIT_TAG v0.12.0
	)
	FetchContent_MakeAvailable(httplib)
endif()

# Main library
add_library(metricz
		src/metrics.cpp
)

target_include_directories(metricz
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
		PRIVATE
		${httplib_SOURCE_DIR}
)

target_link_libraries(metricz PRIVATE httplib)

if(UNIX)
	target_link_libraries(metricz PRIVATE pthread)
endif()

if(ENABLE_EMBEDDED)
	target_compile_definitions(metricz PUBLIC METRICZ_EMBEDDED)
endif()

if(ENABLE_HEADER_ONLY)
	target_compile_definitions(metricz PUBLIC METRICZ_HEADER_ONLY)
endif()

if(BUILD_EXAMPLES)
	# Basic example
	add_executable(metricz_example examples/base_example.cpp)
	target_link_libraries(metricz_example PRIVATE metricz)
	target_include_directories(metricz_example
			PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${httplib_SOURCE_DIR}
	)

	# Exporters example
	add_executable(exporters_example examples/exporters_example.cpp)
	target_link_libraries(exporters_example PRIVATE metricz httplib)
	target_include_directories(exporters_example
			PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${httplib_SOURCE_DIR}
	)
endif()

include(GNUInstallDirs)
install(TARGETS metricz
		EXPORT metricz-targets
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES
		include/metrics.hpp
		include/metrics_exporter.hpp
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/metricz
)

install(EXPORT metricz-targets
		FILE metricz-targets.cmake
		NAMESPACE metricz::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metricz
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/metricz-config-version.cmake"
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
)

# Package configuration
configure_package_config_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/metricz-config.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/metricz-config.cmake"
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metricz
)

install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/metricz-config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/metricz-config-version.cmake"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metricz
)
