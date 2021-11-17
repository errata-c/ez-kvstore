#include "KVPrivate.hpp"

#include <cassert>
#include <iostream>
#include <fmt/core.h>

#include "xxhash.h"

namespace ez {
	// I have no choice but to implement hashing manually for the data blobs.
	// SQLite does not support easily selecting blobs.
	// 
	// Define a simple hashing function, that handles the conversions for us
	int64_t hashStr(std::string_view data) {
		union {
			int64_t ival;
			uint64_t uval;
		} convert;

		convert.uval = XXH3_64bits(data.data(), data.length());
		return convert.ival;
	}

	// The id is the first 8 hex values of the sha256 hash of "ez-kvstore", 0xCB4D74FF
	static constexpr int32_t application_id = 0xCB4D74FF;


	bool KVPrivate::isOpen() const noexcept {
		return db.has_value();
	}
	bool KVPrivate::create(const std::filesystem::path& path, bool overwrite) {
		if (isOpen()) {
			return false;
		}
		namespace fs = std::filesystem;
		fs::file_status status = fs::status(path);
		if (fs::exists(status)) {
			if (!overwrite) {
				return false;
			}

			if (fs::is_regular_file(status)) {
				if (!fs::remove(path)) {
					return false;
				}
			}
			else {
				return false;
			}
		}

		// Create the database itself
		try {
			db.emplace(path.u8string(), SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
		}
		catch (std::exception& e) {
			std::cerr << "Failed to create a sqlite database with error:\n";
			std::cerr << e.what();
			return false;
		}

		// Create the main key-value store table
		try {
			SQLite::Statement stmt{
				db.value(),
				"CREATE TABLE ez_kvstore(\"hash\" INTEGER NOT NULL UNIQUE, \"key\" BLOB NOT NULL, \"value\" BLOB NOT NULL, PRIMARY KEY(\"hash\"));"
			};

			stmt.executeStep();
		}
		catch (std::exception& e) {
			std::string err = fmt::format(
				"ez::KVStore failed to create the main table for an sqlite database with error:\n{}\n", e.what());
			throw std::logic_error(err);
		}

		// Set the application_id pragma, so we can identify the database correctly when opening.
		
		try {
			// For some reason this works, while the binding does not...
			std::string query = fmt::format("PRAGMA main.application_id = {};", application_id);
			SQLite::Statement stmt{
				db.value(),
				query
			};
			
			stmt.executeStep();
		}
		catch (std::exception &e) {
			std::string err = fmt::format(
				"ez::KVStore failed to set the application_id for an sqlite database with error:\n{}\n", e.what());
			throw std::logic_error(err);
		}

		// Create the meta information table
		try {
			SQLite::Statement stmt{
				db.value(),
				"CREATE TABLE ez_kvstore_meta(\"key\" TEXT NOT NULL UNIQUE, \"value\" BLOB NOT NULL, PRIMARY KEY(\"key\"));"
			};

			stmt.executeStep();
		}
		catch (std::exception& e) {
			std::string err = fmt::format(
				"ez::KVStore failed to create the meta table for an sqlite database with error:\n{}\n", e.what());
			throw std::logic_error(err);
		}

		// Create index for better performance
		try {
			SQLite::Statement stmt{
				db.value(),
				"CREATE INDEX kvidx ON ez_kvstore(\"hash\");"
			};

			stmt.executeStep();
		}
		catch (std::exception& e) {
			std::string err = fmt::format(
				"ez::KVStore failed to index main the table for an sqlite database with error:\n{}\n", e.what());
			throw std::logic_error(err);
		}

		// Modify pragmas
		try {
			SQLite::Statement stmt{
				db.value(),
				"PRAGMA main.synchronous = 1;"
			};

			stmt.executeStep();
		}
		catch (std::exception& e) {
			std::string err = fmt::format(
				"ez::KVStore failed to set main.synchronous with error:\n{}\n", e.what());
			throw std::logic_error(err);
		}

		return true;
	}
	bool KVPrivate::open(const std::filesystem::path& path, bool readonly) {
		if (isOpen()) {
			return false;
		}
		namespace fs = std::filesystem;
		fs::file_status status = fs::status(path);

		if (fs::exists(status)) {
			if (fs::is_regular_file(status)) {
				int flags = SQLite::OPEN_READWRITE;
				if (readonly) {
					flags = SQLite::OPEN_READONLY;
				}
				db.emplace(path.u8string(), flags);

				int32_t app_id = 0;
				// Verify that the database is actually something we can use.
				try {
					SQLite::Statement stmt{
						db.value(),
						"PRAGMA main.application_id;"
					};

					stmt.executeStep();
					app_id = stmt.getColumn(0);
				}
				catch (std::exception & e) {
					std::string err = fmt::format(
						"ez::KVStore failed to query the application_id for the database, with error:\n{}\n", e.what());
					throw std::logic_error(err);
				}

				if (app_id != application_id) {
					db.reset();
					return false;
				}

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
	void KVPrivate::close() {
		if (isOpen()) {
			batch.reset();

			// Maybe run PRAGMA optimize?
			/*
			{
				SQLite::Statement stmt{
					db.value(),
					"PRAGMA optimize;"
				};
				stmt.executeStep();
			}
			/**/

			containsStmt.reset();
			getStmt.reset();
			setStmt.reset();
			eraseStmt.reset();
			db.reset();
		}
	}

	bool KVPrivate::getKind(std::string& kind) {
		if (!isOpen()) {
			return false;
		}

		SQLite::Statement stmt{
			db.value(),
			"SELECT \"value\" FROM ez_kvstore_meta WHERE \"key\" = \"kind\";"
		};

		stmt.executeStep();

		if (stmt.hasRow()) {
			SQLite::Column col = stmt.getColumn(0);
			kind.assign((const char *)col.getBlob(), col.getBytes());
			return true;
		}
		else {
			return false;
		}
	}
	bool KVPrivate::setKind(std::string_view kind) {
		if (!isOpen()) {
			return false;
		}

		SQLite::Statement stmt{
			db.value(),
			"INSERT INTO ez_kvstore_meta(\"key\", \"value\") VALUES (\"kind\", ?) ON CONFLICT(\"key\") DO UPDATE SET \"value\"=excluded.\"value\";"
		};
		stmt.bind(1, kind.data(), kind.length());

		stmt.executeStep();

		return true;
	}

	bool KVPrivate::empty() const noexcept {
		return size() == 0;
	}
	std::size_t KVPrivate::size() const noexcept {
		if (db) {
			if (!countStmt) {
				countStmt.emplace(
					db.value(),
					"SELECT COUNT(*) FROM ez_kvstore;"
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
					"SELECT 1 WHERE EXISTS (SELECT * FROM ez_kvstore WHERE \"hash\"=?)"
				);
			}
			SQLite::Statement& stmt = containsStmt.value();

			stmt.bind(1, hashStr(name));
			bool res = stmt.executeStep();
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
					"SELECT \"value\" FROM ez_kvstore WHERE \"hash\"=?"
				);
			}
			SQLite::Statement& stmt = getStmt.value();

			stmt.bind(1, hashStr(name));
			if (stmt.executeStep()) {
				SQLite::Column col = stmt.getColumn(0);
				data.assign((const char*)col.getBlob(), col.getBytes());
				
				stmt.reset();
				return true;
			}
			else {
				stmt.reset();
				return false;
			}
		}
		else {
			return false;
		}
	}
	bool KVPrivate::set(std::string_view key, std::string_view value) {
		if (db) {
			if (!setStmt) {
				setStmt.emplace(
					db.value(),
					"INSERT INTO ez_kvstore (\"hash\", \"key\", \"value\") VALUES (?, ?, ?) ON CONFLICT(\"hash\") DO UPDATE SET \"value\"=excluded.\"value\";"
				);
			}
			SQLite::Statement& stmt = setStmt.value();
			stmt.bind(1, hashStr(key));
			stmt.bind(2, key.data(), key.length());
			stmt.bind(3, value.data(), value.length());
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
					"DELETE FROM ez_kvstore WHERE \"hash\" = ?; SELECT changes();"
				);
			}
			SQLite::Statement& stmt = eraseStmt.value();
			stmt.bind(1, hashStr(name));
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