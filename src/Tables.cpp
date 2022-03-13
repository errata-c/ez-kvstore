#include "KVPrivate.hpp"

#include <cassert>
#include <iostream>
#include <fmt/core.h>

namespace ez {
	// The default table is initially set to 'main'
	// The name ez_kvstore_meta is reserved for internal use!

	std::size_t KVPrivate::numTables() const {
		if (!db) {
			return 0;
		}

		SQLite::Statement stmt(
			db.value(),
			"SELECT COUNT(*) FROM ez_kvstore_tables;"
		);

		bool res = stmt.executeStep();
		assert(res);

		int64_t count = stmt.getColumn(0).getInt64();
		assert(count >= 1);

		// Subtract the meta table from the count
		return static_cast<std::size_t>(count) - 1;
	}
	bool KVPrivate::containsTable(std::string_view name) const {
		if (!db) {
			return false;
		}

		SQLite::Statement stmt(
			db.value(),
			"SELECT COUNT(*) FROM ez_kvstore_tables WHERE 'hash' = ?;"
		);

		int64_t hv = kvhash(name);
		stmt.bind(1, hv);
		bool res = stmt.executeStep();
		assert(res);

		int count = stmt.getColumn(0).getInt();
		return count == 1;
	}
	bool KVPrivate::getTable(std::string& name) const {
		if (!db) {
			return false;
		}

		name = currentTable;

		return true;
	}

	static std::string CreateTableID(std::string_view name) {
		int64_t hv = kvhash(name);
		return fmt::format("ez_{:X}", static_cast<uint64_t>(hv));
	}
	bool KVPrivate::createTable(std::string_view name) {
		if (!db) {
			return false;
		}
		if (containsTable(name)) {
			return false;
		}

		currentTable = name;
		tableID = CreateTableID(name);

		{
			SQLite::Statement stmt(
				db.value(),
				fmt::format(
					"CREATE TABLE \"{}\"("
					"\"hash\" INTEGER UNIQUE, "
					"\"key\" BLOB NOT NULL, "
					"\"value\" BLOB NOT NULL, "
					"PRIMARY KEY(\"hash\"));",
					tableID
				)
			);

			stmt.executeStep();
		}

		{
			SQLite::Statement stmt(
				db.value(),
				"INSERT INTO ez_kvstore_tables(\"hash\", \"name\") VALUES (?, ?);"
			);

			stmt.bind(1, kvhash(name));
			stmt.bind(2, currentTable);

			stmt.executeStep();
		}

		containsStmt.reset();
		getStmt.reset();
		setStmt.reset();
		eraseStmt.reset();
		countStmt.reset();

		return true;
	}
	bool KVPrivate::setTable(std::string_view name) {
		if (!db) {
			return false;
		}
		if (!containsTable(name)) {
			return false;
		}

		currentTable = name;
		tableID = CreateTableID(name);

		containsStmt.reset();
		getStmt.reset();
		setStmt.reset();
		eraseStmt.reset();
		countStmt.reset();

		return true;
	}
	bool KVPrivate::setDefaultTable(std::string_view name) {
		if (!db) {
			return false;
		}
		if (!containsTable(name)) {
			return false;
		}
		
		SQLite::Statement stmt{
			db.value(),
			"INSERT INTO ez_kvstore_meta(\"key\", \"value\") "
			"VALUES (\"default_table\", ?) ON CONFLICT(\"key\") DO UPDATE SET \"value\"=excluded.\"value\";"
		};
		stmt.bind(1, name.data(), name.length());

		stmt.executeStep();

		return true;
	}
	bool KVPrivate::getDefaultTable(std::string& name) const {
		if (!db) {
			return false;
		}
		
		SQLite::Statement stmt{
			db.value(),
			"SELECT \"value\" FROM ez_kvstore_meta WHERE \"key\" = \"default_table\";"
		};

		if (stmt.executeStep()) {
			SQLite::Column col = stmt.getColumn(0);
			name.assign((const char*)col.getBlob(), col.getBytes());
			return true;
		}
		else {
			return false;
		}
	}
	bool KVPrivate::eraseTable(std::string_view name) {
		if (!db) {
			return false;
		}
		if (name == currentTable) {
			return false;
		}
		if (!containsTable(name)) {
			return false;
		}

		SQLite::Statement stmt(
			db.value(),
			fmt::format(
				"DROP TABLE \"{}\"; "
				"DELETE FROM ez_kvstore_tables WHERE \"hash\" = ?; SELECT changes();",
				tableID
			)
		);

		stmt.bind(1, kvhash(name));
		stmt.executeStep();

		return true;
	}
}