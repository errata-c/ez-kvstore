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
		assert(count > 0);

		// Subtract the meta table from the count
		return static_cast<std::size_t>(count);
	}
	bool KVPrivate::containsTable(std::string_view name) const {
		if (!db) {
			return false;
		}

		SQLite::Statement stmt(
			db.value(),
			"SELECT COUNT(*) FROM ez_kvstore_tables WHERE \"hash\" = ?;"
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
	bool KVPrivate::renameTable(std::string_view old, std::string_view name) {
		if (!db) {
			return false;
		}

		int64_t oldhv = kvhash(old);
		int64_t namehv = kvhash(name);

		{
			SQLite::Statement stmt(
				db.value(),
				"SELECT \"hash\" FROM ez_kvstore_tables WHERE \"hash\" = ? OR \"hash\" = ?;"
			);

			stmt.bind(1, oldhv);
			stmt.bind(2, namehv);

			int count = 0;
			int64_t val[2];
			while (stmt.executeStep()) {
				val[count] = stmt.getColumn(0).getInt64();
				++count;
			}
			
			// If the old table doesn't exist, then return false
			// If the new table exists, then return false
			bool found = false;
			for (int i = 0; i < count; ++i) {
				if (val[i] == oldhv) {
					found = true;
				}
				if (val[i] == namehv) {
					return false;
				}
			}
			if (!found) {
				return false;
			}
		}

		std::string nid = CreateTableID(name);

		{
			SQLite::Statement stmt(
				db.value(),
				fmt::format(
					"ALTER TABLE \"{}\" RENAME TO \"{}\";",
					CreateTableID(old),
					nid
				)
			);
			stmt.exec();
		}

		{
			SQLite::Statement stmt(
				db.value(),
				"DELETE FROM ez_kvstore_tables WHERE \"hash\" = ?;"
			);

			stmt.bind(1, oldhv);
			stmt.executeStep();
		}

		{
			SQLite::Statement stmt(
				db.value(),
				"INSERT INTO ez_kvstore_tables(\"hash\", \"name\") VALUES (?, ?);"
			);

			stmt.bind(1, namehv);
			stmt.bind(2, name.data(), name.length());
			stmt.executeStep();
		}

		// Make sure the table is updated if needed.
		if (old == currentTable) {
			bool err = setTable(name);
			assert(err);
		}

		return true;
	}
}