#include <catch2/catch.hpp>

#include <fmt/core.h>

#include <istream>
#include <ostream>

#include <ez/BufferStream.hpp>

TEST_CASE("string as buffer") {
	const char str[] = "Just testing this.";

	ez::intern::memorybuf buff{ str, sizeof(str)-1 };
	std::istream in(&buff);
	std::string line;
	REQUIRE(std::getline(in, line));
	REQUIRE(line == str);
}

TEST_CASE("IBufferStream") {
	const char str[] = "Just testing this.";

	ez::IBufferStream in{ str, sizeof(str) - 1 };
	std::string line;
	REQUIRE(std::getline(in, line));
	REQUIRE(line == str);
}

TEST_CASE("immutable test") {
	// It is impossible to edit the contents of the streambuf, as the right methods have not been implemented.
	// This does make them const correct, but there is no error checking to make sure its being done right.
	std::string str(64, '\0');
	ez::intern::memorybuf buff{ str.data(), str.size() };
	std::ostream out(&buff);

	fmt::format_to(std::ostreambuf_iterator<char>(out), "Just testing this.");
	
	for (int i = 0; i < 64; ++i) {
		REQUIRE(str[i] == '\0');
	}
}