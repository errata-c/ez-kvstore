#include "KVPrivate.hpp"

#include <cassert>
#include <iostream>
#include <fmt/core.h>

namespace ez {
	std::size_t KVPrivate::numValues() const {
		if (!db) {
			return 0;
		}

		SQLite::Statement stmt(
			db.value(),
			fmt::format(
				"SELECT COUNT(*) FROM \"{}\";",
				tableID
			)
		);
		bool res = stmt.executeStep();
		assert(res == true);

		int64_t val = stmt.getColumn(0).getInt64();
		assert(val >= 0);
		return static_cast<std::size_t>(val);
	}

	bool KVPrivate::contains(std::string_view name) const {
		if (!db) {
			return false;
		}

		if (!containsStmt) {
			containsStmt.emplace(
				db.value(),
				fmt::format(
					"SELECT 1 WHERE EXISTS (SELECT * FROM \"{}\" WHERE \"hash\" = ?);",
					tableID
				)
			);
		}
		else {
			containsStmt.value().reset();
		}

		SQLite::Statement& stmt = containsStmt.value();

		stmt.bind(1, kvhash(name));

		return stmt.executeStep();
	}

	bool KVPrivate::getRaw(std::string_view name, const void*& data, std::size_t& len) const {
		if (db) {
			if (!getStmt) {
				getStmt.emplace(
					db.value(),
					fmt::format(
						"SELECT \"value\" FROM \"{}\" WHERE \"hash\" = ?",
						tableID
					)
				);
			}
			else {
				getStmt.value().reset();
			}

			SQLite::Statement& stmt = getStmt.value();

			stmt.bind(1, kvhash(name));
			if (stmt.executeStep()) {
				SQLite::Column col = stmt.getColumn(0);
				data = col.getBlob();
				len = static_cast<std::size_t>(col.getBytes());

				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	bool KVPrivate::setRaw(std::string_view key, const void* data, std::size_t len) {
		if (!db) {
			return false;
		}
		
		if (!setStmt) {
			setStmt.emplace(
				db.value(),
				fmt::format(
					"INSERT INTO \"{}\" (\"hash\", \"key\", \"value\") "
					"VALUES (?, ?, ?) ON CONFLICT(\"hash\") "
					"DO UPDATE SET \"value\"=excluded.\"value\";",
					tableID
				)
			);
		}
		else {
			setStmt.value().reset();
		}

		SQLite::Statement& stmt = setStmt.value();

		stmt.bind(1, kvhash(key));
		stmt.bind(2, key.data(), key.length());
		stmt.bind(3, data, len);
		bool res = stmt.executeStep();
		assert(res == false);

		return true;
	}


	bool KVPrivate::erase(std::string_view name) {
		if (db) {
			if (!eraseStmt) {
				eraseStmt.emplace(
					db.value(),
					fmt::format(
						"DELETE FROM \"{}\" WHERE \"hash\" = ?; SELECT changes();",
						tableID
					)
				);
			}
			else {
				eraseStmt.value().reset();
			}

			SQLite::Statement& stmt = eraseStmt.value();

			stmt.bind(1, kvhash(name));
			bool res = stmt.executeStep();
			assert(res);

			return stmt.getColumn(0).getInt() == 1;
		}
		else {
			return false;
		}
	}
}