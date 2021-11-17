#include <catch2/catch.hpp>

#include <fmt/core.h>

#include <istream>
#include <ostream>

#include <ez/memstream.hpp>

TEST_CASE("string as buffer") {
	const char str[] = "Just testing this.";

	ez::membuf buff{ str, sizeof(str)-1 };
	std::istream in(&buff);
	std::string line;
	REQUIRE(std::getline(in, line));
	REQUIRE(line == str);
}

TEST_CASE("imemstream") {
	const char str[] = "Just testing this.";

	ez::imemstream in{ str, sizeof(str) - 1 };
	std::string line;
	REQUIRE(std::getline(in, line));
	REQUIRE(line == str);
}

TEST_CASE("immutable test") {
	// It is impossible to edit the contents of the streambuf, as the right methods have not been implemented.
	// This does make them const correct, but there is no error checking to make sure its being done right.
	std::string str(64, '\0');
	ez::membuf buff{ str.data(), str.size() };
	std::ostream out(&buff);

	fmt::format_to(std::ostreambuf_iterator<char>(out), "Just testing this.");
	
	for (int i = 0; i < 64; ++i) {
		REQUIRE(str[i] == '\0');
	}
}

TEST_CASE("swapping membuf") {
	char arr0[] = "testing";
	char arr1[] = "something";

	ez::membuf buf0{ arr0, sizeof(arr0) - 1 };
	ez::membuf buf1{ arr1, sizeof(arr1) - 1 };

	buf0.swap(buf1);

	std::istream in0(&buf0), in1(&buf1);
	std::string v0, v1;
	std::getline(in0, v0);
	std::getline(in1, v1);

	REQUIRE(v0 == "something");
	REQUIRE(v1 == "testing");
}