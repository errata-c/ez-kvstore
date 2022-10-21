#include <ez/intern/KVGenerators.hpp>

#include <fmt/format.h>

namespace ez {
	KVEntryGenerator::KVEntryGenerator(SQLite::Database& db, std::string_view table)
		: stmt(
			db,
			fmt::format(
				"SELECT \"key\", \"value\" FROM \"{}\";",
				table
			)
		)
	{}
	bool KVEntryGenerator::advance(KVEntry& value) {
		if (stmt.executeStep()) {
			{
				SQLite::Column col = stmt.getColumn(0);
				value.key.assign((const char*)col.getBlob(), col.getBytes());
			}
			{
				SQLite::Column col = stmt.getColumn(1);
				value.value.assign((const char*)col.getBlob(), col.getBytes());
			}
			return true;
		}
		else {
			return false;
		}
	}


	KVEntryViewGenerator::KVEntryViewGenerator(SQLite::Database& db, std::string_view table)
		: stmt(
			db,
			fmt::format(
				"SELECT \"key\", \"value\" FROM \"{}\";",
				table
			)
		)
	{}
	bool KVEntryViewGenerator::advance(KVEntryView& value) {
		if (stmt.executeStep()) {
			{
				SQLite::Column col = stmt.getColumn(0);
				value.key = std::string_view((const char*)col.getBlob(), col.getBytes());
			}
			{
				SQLite::Column col = stmt.getColumn(1);
				value.value = std::string_view((const char*)col.getBlob(), col.getBytes());
			}
			return true;
		}
		else {
			return false;
		}
	}
}