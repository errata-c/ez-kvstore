#include <ez/KVStore.hpp>

#include <cassert>
#include <iostream>
#include <fmt/core.h>
#include "hashing.hpp"

namespace ez {
	std::size_t KVStore::numValues() const {
		if (!data->db) {
			return 0;
		}

		SQLite::Statement stmt(
			data->db.value(),
			"SELECT COUNT(*) FROM \"main\";"
		);
		bool res = stmt.executeStep();
		assert(res == true);

		int64_t val = stmt.getColumn(0).getInt64();
		assert(val >= 0);
		return static_cast<std::size_t>(val);
	}

	bool KVStore::contains(std::string_view name) const {
		if (!data->db) {
			return false;
		}

		if (!data->containsStmt) {
			data->containsStmt.emplace(
				data->db.value(),
				"SELECT 1 WHERE EXISTS (SELECT * FROM \"main\" WHERE \"hash\" = ?);"
			);
		}
		else {
			data->containsStmt.value().reset();
		}

		SQLite::Statement& stmt = data->containsStmt.value();

		stmt.bind(1, kvhash(name));

		return stmt.executeStep();
	}

	bool KVStore::getRaw(std::string_view name, const void*& raw, std::size_t& len) const {
		if (data->db) {
			if (!data->getStmt) {
				data->getStmt.emplace(
					data->db.value(),
					"SELECT \"value\" FROM \"main\" WHERE \"hash\" = ?"
				);
			}
			else {
				data->getStmt.value().reset();
			}

			SQLite::Statement& stmt = data->getStmt.value();

			stmt.bind(1, kvhash(name));
			if (stmt.executeStep()) {
				SQLite::Column col = stmt.getColumn(0);
				raw = col.getBlob();
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

	bool KVStore::setRaw(std::string_view key, const void* raw, std::size_t len) {
		if (!data->db) {
			return false;
		}
		
		if (!data->setStmt) {
			data->setStmt.emplace(
				data->db.value(),
				"INSERT INTO \"main\" (\"hash\", \"key\", \"value\") "
				"VALUES (?, ?, ?) ON CONFLICT(\"hash\") "
				"DO UPDATE SET \"value\"=excluded.\"value\";"
			);
		}
		else {
			data->setStmt.value().reset();
		}

		SQLite::Statement& stmt = data->setStmt.value();

		stmt.bind(1, kvhash(key));
		stmt.bind(2, key.data(), key.length());
		stmt.bind(3, raw, len);
		bool res = stmt.executeStep();
		assert(res == false);

		return true;
	}


	bool KVStore::erase(std::string_view name) {
		if (data->db) {
			if (!data->eraseStmt) {
				data->eraseStmt.emplace(
					data->db.value(),
					"DELETE FROM \"main\" WHERE \"hash\" = ?;"
				);
			}
			else {
				data->eraseStmt.value().reset();
			}

			SQLite::Statement& stmt = data->eraseStmt.value();

			stmt.bind(1, kvhash(name));

			return stmt.exec() == 1;
		}
		else {
			return false;
		}
	}

	void KVStore::clear() {
		if (!data->db) {
			return;
		}

		SQLite::Statement stmt(
			data->db.value(),
			"DELETE FROM \"main\";"
		);
		stmt.exec();
	}

	bool KVStore::rename(std::string_view old, std::string_view name) {
		if (!data->db || contains(name)) {
			return false;
		}

		SQLite::Statement stmt(
			data->db.value(),
			"UPDATE \"main\" SET \"hash\" = ?, \"key\" = ? WHERE \"hash\" = ?;"
		);

		int64_t oldhv = kvhash(old);
		int64_t namehv = kvhash(name);

		stmt.bind(1, namehv);
		stmt.bind(2, (const void*)name.data(), name.length());
		stmt.bind(3, oldhv);

		return stmt.exec() == 1;
	}
}