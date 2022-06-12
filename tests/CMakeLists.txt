cmake_minimum_required(VERSION 3.16)

find_package(Catch2 CONFIG REQUIRED)

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