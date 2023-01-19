cmake_minimum_required(VERSION 3.24)

project(EZ_KVSTORE
VERSION 0.2.1
LANGUAGES C CXX)

set(PACKAGE_NAME "ez-kvstore")
set(TARGET_NAME "kvstore")

include(GNUInstallDirs)
include(FetchContent)

set(${PROJECT_NAME}_CONFIG_DIR "${CMAKE_INSTALL_DATAROOTDIR}/${PACKAGE_NAME}" CACHE STRING "The relative directory to install package config files.")

FetchContent_Declare(
	ez-cmake
	GIT_REPOSITORY "https://github.com/errata-c/ez-cmake.git"
	GIT_TAG "main"
)
FetchContent_Declare(
	SQLiteCpp
	GIT_REPOSITORY "https://github.com/SRombauts/SQLiteCpp.git"
	GIT_TAG "3.2.1"
	FIND_PACKAGE_ARGS CONFIG
)
FetchContent_Declare(
	fmt
	GIT_REPOSITORY "https://github.com/fmtlib/fmt"
	GIT_TAG "9.1.0"
	FIND_PACKAGE_ARGS CONFIG
)
FetchContent_Declare(
	xxHash
	GIT_REPOSITORY "https://github.com/Cyan4973/xxHash.git"
	GIT_TAG "v0.8.1"
	FIND_PACKAGE_ARGS CONFIG
)

FetchContent_MakeAvailable(ez-cmake SQLiteCpp fmt xxHash)

add_library(${TARGET_NAME} STATIC
	"src/KVStore.cpp"
	"src/KVStoreValues.cpp"
	"src/KVGenerators.cpp"
	
	"src/hashing.cpp"
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



# Conditional installation logic, we don't want to do this if this is a subproject.
if(PROJECT_IS_TOP_LEVEL)
	include(CTest)
	# Build the tests once the option is enabled.
	if(BUILD_TESTING)
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
	endif()

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