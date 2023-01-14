cmake_minimum_required(VERSION 3.24)

FetchContent_Declare(
	Catch2
	GIT_REPOSITORY "https://github.com/catchorg/Catch2.git"
	GIT_TAG "v3.2.1"
	FIND_PACKAGE_ARGS CONFIG REQUIRED
)

FetchContent_MakeAvailable(Catch2)


configure_file("config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config.hpp" 
	@ONLY
)

add_executable(basic_test 
	"basic/streambuf.cpp"
	"basic/kvstore.cpp"

	"${CMAKE_CURRENT_BINARY_DIR}/config.hpp" 
)
target_link_libraries(basic_test PRIVATE 
	ez::kvstore
	Catch2::Catch2WithMain
	fmt::fmt
)
target_include_directories(basic_test PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}"
)