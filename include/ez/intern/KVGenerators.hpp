#pragma once
#include <ez/intern/KVEntry.hpp>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

namespace ez {
	class KVEntryGenerator {
	public:
		KVEntryGenerator(SQLite::Database& db, std::string_view table);

		bool advance(KVEntry& value);

		SQLite::Statement stmt;
	};
	class KVEntryViewGenerator {
	public:
		KVEntryViewGenerator(SQLite::Database& db, std::string_view table);

		bool advance(KVEntryView& value);

		SQLite::Statement stmt;
	};
}