#include "KVPrivate.hpp"

#include <cassert>
#include <iostream>
#include <fmt/core.h>

namespace ez {
	// I have no choice but to implement hashing manually for the data blobs.
	// SQLite does not support easily selecting blobs.

	// The id is the first 8 hex values of the sha256 hash of "ez-kvstore", 0xCB4D74FF
	static constexpr int32_t application_id = 0xCB4D74FF;

	KVPrivate::KVPrivate()
	{}

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

		// Create the lookup for the managed tables
		{
			SQLite::Statement stmt{
				db.value(),
				"CREATE TABLE ez_kvstore_tables("
					"\"hash\" INTEGER PRIMARY KEY, "
					"\"name\" BLOB NOT NULL);"
			};

			stmt.executeStep();
		}

		// Set the application_id pragma, so we can identify the database correctly when opening.
		{
			// For some reason this works, while the binding does not...
			SQLite::Statement stmt{
				db.value(),
				fmt::format(
					"PRAGMA main.application_id = {};", 
					application_id
				)
			};
			
			stmt.executeStep();
		}

		// Create the meta information table
		{
			SQLite::Statement stmt{
				db.value(),
				"CREATE TABLE ez_kvstore_meta("
					"\"key\" TEXT NOT NULL UNIQUE, "
					"\"value\" BLOB NOT NULL, PRIMARY KEY(\"key\"));"
			};

			stmt.executeStep();
		}

		// Set the kind value to a default
		bool result = setKind("ez_kvstore");
		assert(result);

		// Create the first table, name it 'main'
		result = createTable("main");
		assert(result);

		// Make it the default table
		result = setDefaultTable("main");
		assert(result);

		// Indexing is not required!
		// The primary key is an integer

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

		if (!fs::exists(status) || !fs::is_regular_file(status)) {
			return false;
		}

		int flags = SQLite::OPEN_READWRITE;
		if (readonly) {
			flags = SQLite::OPEN_READONLY;
		}
		db.emplace(path.u8string(), flags);

		int32_t app_id = 0;
		// Verify that the database is actually something we can use.
		{
			SQLite::Statement stmt{
				db.value(),
				"PRAGMA main.application_id;"
			};
			stmt.executeStep();
			app_id = stmt.getColumn(0);
		}

		if (app_id != application_id) {
			db.reset();
			return false;
		}

		// open the default table
		std::string table;
		if (!getDefaultTable(table)) {
			db.reset();
			return false;
		}

		if (!setTable(table)) {
			db.reset();
			return false;
		}

		return true;
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

	bool KVPrivate::getKind(std::string& kind) const {
		if (!isOpen()) {
			return false;
		}

		SQLite::Statement stmt{
			db.value(),
			"SELECT \"value\" FROM ez_kvstore_meta WHERE \"key\" = \"kind\";"
		};

		if (stmt.executeStep()) {
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
		return numValues() == 0;
	}
	

	bool KVPrivate::inBatch() const {
		return bool(batch);
	}
	bool KVPrivate::beginBatch() {
		if (!isOpen() || inBatch()) {
			return false;
		}

		batch.emplace(db.value());

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
}