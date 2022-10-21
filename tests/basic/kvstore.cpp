#include <catch2/catch_all.hpp>

#include "config.hpp"

#include <ez/KVStore.hpp>

#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;


TEST_CASE("writing") {
	fs::path path = test_dir;
	path /= "write.db3";

	ez::KVStore store;

	REQUIRE(!store.isOpen());

	REQUIRE(store.create(path, true));

	REQUIRE(store.numValues() == 0);

	REQUIRE(store.set("hello", "world"));
	REQUIRE(store.numValues() == 1);

	REQUIRE(store.set("what", "fun"));
	REQUIRE(store.numValues() == 2);

	REQUIRE(store.contains("hello"));
	REQUIRE(store.contains("what"));

	{
		{
			auto it = store.begin();
			auto end = store.end();

			REQUIRE(it != end);
			REQUIRE(it == it);
			REQUIRE(end == end);

			REQUIRE((++it) != end);
			REQUIRE((++it) == end);
		}

		// No guarantee on the order the entries will be iterated over.
		std::unordered_set<std::string> keys, values;
		for (const ez::KVEntryView& entry : store) {
			keys.insert(std::string(entry.key));
			values.insert(std::string(entry.value));
		}

		REQUIRE(keys.count("hello") == 1);
		REQUIRE(keys.count("what") == 1);
		REQUIRE(values.count("world") == 1);
		REQUIRE(values.count("fun") == 1);
	}

	store.clear();

	REQUIRE(store.numValues() == 0);

	REQUIRE(store.set("test", "something"));

	REQUIRE(store.contains("test"));
	REQUIRE(store.rename("test", "got"));
	REQUIRE(store.contains("got"));
	REQUIRE(!store.contains("test"));
	REQUIRE(store.numValues() == 1);
}

TEST_CASE("reading") {
	fs::path path = test_dir;
	path /= "read.db3";

	ez::KVStore store;
	
	REQUIRE(!store.isOpen());

	std::string value;
	REQUIRE(!store.get("test", value));
	REQUIRE(!store.set("test", value));
	REQUIRE(!store.erase("test"));
	REQUIRE(!store.contains("test"));
	REQUIRE(store.numValues() == 0);

	REQUIRE(store.open(path, true));

	// Call some of these methods twice in a row to make sure the resets are working properly.
	REQUIRE(store.contains("hello"));
	REQUIRE(store.contains("what"));

	REQUIRE(store.get("hello", value));
	REQUIRE(value == "world");

	REQUIRE(store.get("what", value));
	REQUIRE(value == "fun");

	REQUIRE(store.numValues() == 2);

	ez::imemstream in;
	REQUIRE(store.getStream("hello", in));
	
	REQUIRE(std::getline(in, value));
	REQUIRE(value == "world");

	in.reset();
}


