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

	bool res = store.open(path);
	REQUIRE(res);
}