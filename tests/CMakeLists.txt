cmake_minimum_required(VERSION 3.16)

find_package(Catch2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(basic_test 
	"basic/main.cpp"
	"basic/streambuf.cpp"
)
target_link_libraries("basic_test" PRIVATE 
	ez::kvstore
	Catch2::Catch2
	fmt::fmt
)
