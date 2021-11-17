#include "KVPrivate.hpp"

#include <cassert>

namespace ez {
	bool KVPrivate::isOpen() const noexcept {
		return db.has_value();
	}
	bool KVPrivate::create(const std::filesystem::path& path) {
		if (isOpen()) {
			return false;
		}
		namespace fs = std::filesystem;
		fs::file_status status = fs::status(path);

		return false;
	}
	bool KVPrivate::open(const std::filesystem::path& path, bool readonly) {
		if (isOpen()) {
			return false;
		}
		namespace fs = std::filesystem;
		fs::file_status status = fs::status(path);

		if (fs::exists(status)) {
			
			return false;
		}
		else {
			return false;
		}
	}
	void KVPrivate::close() {
		if (isOpen()) {
			batch.reset();
			containsStmt.reset();
			getStmt.reset();
			setStmt.reset();
			eraseStmt.reset();
			db.reset();
		}
	}

	bool KVPrivate::empty() const noexcept {
		return size() == 0;
	}
	std::size_t KVPrivate::size() const noexcept {
		if (db) {
			if (!countStmt) {
				countStmt.emplace(
					db.value(),
					"SELECT COUNT(*) FROM ez_kvstore_table;"
				);
			}
			SQLite::Statement& stmt = countStmt.value();
			bool res = stmt.executeStep();
			assert(res == false);

			int64_t val = stmt.getColumn(0).getInt64();
			assert(val >= 0);
			stmt.reset();
			return static_cast<std::size_t>(val);
		}
		else {
			return 0;
		}
	}

	bool KVPrivate::inBatch() const {
		return bool(batch);
	}
	bool KVPrivate::beginBatch() {
		if (!isOpen() || inBatch()) {
			return false;
		}
		return true;
	}
	void KVPrivate::commitBatch() {
		if (!inBatch()) {
			throw std::logic_error("Attempt to commit a batch when not in a batch!");
		}
		batch.value().commit();
		batch.reset();
	}
	void KVPrivate::cancelBatch() {
		batch.reset();
	}

	bool KVPrivate::contains(std::string_view name) const {
		if (db) {
			if (!containsStmt) {
				containsStmt.emplace(
					db.value(),
					"SELECT 1 WHERE EXISTS (SELECT * FROM ez_kvstore_table WHERE \"key\"=?)"
				);
			}
			SQLite::Statement& stmt = containsStmt.value();

			stmt.bind(1, name.data(), name.length());
			bool res = stmt.executeStep();
			assert(res == false);

			res = stmt.hasRow();
			stmt.reset();
			return res;
		}
		else {
			return false;
		}
	}

	bool KVPrivate::get(std::string_view name, std::string& data) const {
		if (db) {
			if (!getStmt) {
				getStmt.emplace(
					db.value(),
					"SELECT \"value\" FROM TABLE ez_kvstore_table WHERE \"key\"=?"
				);
			}
			SQLite::Statement& stmt = getStmt.value();

			stmt.bind(1, name.data(), name.length());
			bool res = stmt.executeStep();
			assert(res == false);

			res = false;
			if (stmt.hasRow()) {
				data = std::string{ stmt.getColumn(0) };
				res = true;
			}

			stmt.reset();
			return res;
		}
		else {
			return false;
		}
	}
	bool KVPrivate::set(std::string_view name, std::string_view data) {
		if (db) {
			if (!setStmt) {
				setStmt.emplace(
					db.value(),
					"INSERT INTO ez_kvstore_table (\"key\", \"value\") VALUES (?, ?) ON CONFLICT(\"key\") DO UPDATE SET \"value\"=excluded.\"value\";"
				);
			}
			SQLite::Statement& stmt = setStmt.value();
			stmt.bind(1, name.data(), name.length());
			stmt.bind(2, data.data(), data.length());
			bool res = stmt.executeStep();
			assert(res == false);

			stmt.reset();
			return true;
		}
		else {
			return false;
		}
	}
	bool KVPrivate::erase(std::string_view name) {
		if (db) {
			if (!eraseStmt) {
				eraseStmt.emplace(
					db.value(),
					"DELETE FROM ez_kvstore_table WHERE \"key\" = ?; SELECT changes();"
				);
			}
			SQLite::Statement& stmt = eraseStmt.value();
			stmt.bind(1, name.data(), name.length());
			bool res = stmt.executeStep();
			assert(res == false);

			res = stmt.getColumn(0).getInt() == 1;
			stmt.reset();
			return res;
		}
		else {
			return false;
		}
	}
}