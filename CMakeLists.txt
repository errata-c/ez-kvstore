cmake_minimum_required(VERSION 3.16)

project(EZ_KVSTORE
VERSION 0.1.0
LANGUAGES C CXX)

set(PACKAGE_NAME "ez-kvstore")
set(TARGET_NAME "kvstore")

include(GNUInstallDirs)
include(FetchContent)

option(${PROJECT_NAME}_BUILD_TESTS "Build the test executables" ON)
set(${PROJECT_NAME}_CONFIG_DIR "${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME}" CACHE STRING "The relative directory to install package config files.")

FetchContent_Declare(
	ez-cmake
	GIT_REPOSITORY "https://github.com/errata-c/ez-cmake.git"
	GIT_TAG "main"
)
FetchContent_MakeAvailable(ez-cmake)

ez_is_project_top_level(${PROJECT_NAME}_TOP_LEVEL)

# Find deps
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

add_library(${TARGET_NAME} STATIC
	"src/KVStore.cpp"
	"src/KVPrivate.cpp"
	"src/Values.cpp"
	"src/Tables.cpp"
	"src/Iterator.cpp"
)

target_include_directories(${TARGET_NAME} PUBLIC 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(${TARGET_NAME} PUBLIC
	SQLiteCpp
	fmt::fmt
	xxHash::xxhash
)

# Select the c++ version to use.
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)

# Just for internal compatibility, so that subprojects can use the namespaced version
add_library("ez::${TARGET_NAME}" ALIAS "${TARGET_NAME}")

# Build the tests once the option is enabled.
if(${${PROJECT_NAME}_BUILD_TESTS})
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

# Conditional installation logic, we don't want to do this if this is a subproject.
if(${${PROJECT_NAME}_TOP_LEVEL})
	install(
		DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
		TYPE INCLUDE
		FILES_MATCHING
		PATTERN "*.h" PATTERN "*.hpp"
	)

	install(TARGETS ${TARGET_NAME}
		EXPORT ${TARGET_NAME}-targets
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/$<CONFIG>"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
	)

	install_package(
		NAME "${PACKAGE_NAME}"
		EXPORT "${TARGET_NAME}-targets"
		VERSION "${PROJECT_VERSION}"

		NAMESPACE "ez::"

		# Version compatiblity
		COMPATIBILITY "SameMajorVersion"

		# Relative install location of the package config files.
		DESTINATION "${${PROJECT_NAME}_CONFIG_DIR}"

		PRECONFIG "cmake/preconfig.cmake"
	)
endif()