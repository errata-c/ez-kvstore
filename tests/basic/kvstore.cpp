#include <catch2/catch_all.hpp>

#include "config.hpp"

#include <ez/KVStore.hpp>

namespace fs = std::filesystem;

TEST_CASE("reading") {
	fs::path path = test_dir;
	path /= "read.db3";

	ez::KVStore store;
	
	REQUIRE(!store.isOpen());
	REQUIRE(!store.inBatch());

	REQUIRE(!store.beginBatch());

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

TEST_CASE("writing") {
	fs::path path = test_dir;
	path /= "write.db3";

	ez::KVStore store;

	REQUIRE(!store.isOpen());
	REQUIRE(!store.inBatch());

	REQUIRE(!store.beginBatch());

	REQUIRE(store.create(path, true));

	REQUIRE(store.numTables() == 1);
	REQUIRE(store.numValues() == 0);

	// Make sure the default table is present
	std::string table;
	REQUIRE(store.getTable(table));

	REQUIRE(table == "main");

	REQUIRE(store.containsTable(table));

	REQUIRE(store.set("hello", "world"));
	REQUIRE(store.numValues() == 1);

	REQUIRE(store.set("what", "fun"));
	REQUIRE(store.numValues() == 2);

	REQUIRE(store.contains("hello"));
	REQUIRE(store.contains("what"));

	REQUIRE(store.createTable("secondary"));
	REQUIRE(store.getTable(table));
	REQUIRE(table == "secondary");
	REQUIRE(store.containsTable(table));

	REQUIRE(store.numTables() == 2);
	REQUIRE(store.numValues() == 0);
	REQUIRE(store.set("something", "cool"));
	REQUIRE(store.numValues() == 1);

	{
		auto it = store.begin(), end = store.end();
		REQUIRE((it != end));

		auto kv = *it;
		REQUIRE(kv.key == "something");

		++it;
		REQUIRE((it == end));
	}

	{
		auto it = store.beginTables(), end = store.endTables();
		REQUIRE((it != end));

		auto val = *it;
		std::string old = val;
		REQUIRE((val == "main" || val == "secondary"));

		++it;
		REQUIRE((it != end));

		val = *it;
		REQUIRE((val == "main" || val == "secondary"));
		REQUIRE(val != old);

		++it;
		REQUIRE((it == end));
	}
	
	REQUIRE(store.renameTable("secondary", "testing"));

	REQUIRE(!store.containsTable("secondary"));
	REQUIRE(store.containsTable("testing"));

	REQUIRE(store.numTables() == 2);
	REQUIRE(store.numValues() == 1);

	store.clear();

	REQUIRE(store.numValues() == 0);

	REQUIRE(store.set("test", "something"));

	REQUIRE(store.contains("test"));
	REQUIRE(store.rename("test", "got"));
	REQUIRE(store.contains("got"));
	REQUIRE(!store.contains("test"));
	REQUIRE(store.numValues() == 1);
}

