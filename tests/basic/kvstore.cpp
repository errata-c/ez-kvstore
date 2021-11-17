#include <catch2/catch.hpp>

#include "config.hpp"

#include <ez/KVStore.hpp>

namespace fs = std::filesystem;

TEST_CASE("kvstore") {
	fs::path path = test_dir;
	path /= "test.db3";

	ez::KVStore store;
	
	REQUIRE(!store.isOpen());
	REQUIRE(!store.inBatch());

	REQUIRE(!store.beginBatch());

	std::string value;
	REQUIRE(!store.get("test", value));
	REQUIRE(!store.set("test", value));
	REQUIRE(!store.erase("test"));
	REQUIRE(!store.contains("test"));
	REQUIRE(store.size() == 0);

	REQUIRE(store.open(path, true));

	// Call some of these methods twice in a row to make sure the resets are working properly.
	REQUIRE(store.contains("hello"));
	REQUIRE(store.contains("what"));

	REQUIRE(store.get("hello", value));
	REQUIRE(value == "world");

	REQUIRE(store.get("what", value));
	REQUIRE(value == "fun");

	REQUIRE(store.size() == 4);
	REQUIRE(!store.empty());
}

