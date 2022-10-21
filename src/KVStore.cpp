#include <ez/KVStore.hpp>

#include <iostream>
#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>

#include "hashing.hpp"

namespace ez {
	// The id is the first 8 hex values of the sha256 hash of "ez-kvstore", 0xCB4D74FF
	static constexpr int32_t application_id = 0xCB4D74FF;

	KVStore::KVStore()
		: data(new Data())
	{}

	void KVStore::swap(KVStore& other) noexcept {
		std::swap(data, other.data);
	}

	bool KVStore::isOpen() const noexcept {
		return data->db.has_value();
	}

	bool KVStore::create(const std::filesystem::path& path, bool overwrite) {
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
			data->db.emplace(path.u8string(), SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
		}
		catch (std::exception& e) {
			std::cerr << "Failed to create a sqlite database with error:\n";
			std::cerr << e.what();
			return false;
		}

		// Set the application_id pragma, so we can identify the database correctly when opening.
		{
			// For some reason this works, while the binding does not...
			SQLite::Statement stmt{
				data->db.value(),
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
				data->db.value(),
				"CREATE TABLE ez_kvstore_meta("
					"\"key\" TEXT NOT NULL UNIQUE, "
					"\"value\" BLOB NOT NULL, PRIMARY KEY(\"key\"));"
			};

			stmt.executeStep();
		}

		// Set the kind value to a default
		setKind("ez_kvstore");
		createTable();

		// Indexing is not required!
		// The primary key is an integer

		// Modify pragmas
		try {
			SQLite::Statement stmt{
				data->db.value(),
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
	bool KVStore::open(const std::filesystem::path& path, bool readonly) {
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
		data->db.emplace(path.u8string(), flags);

		int32_t app_id = 0;
		// Verify that the database is actually something we can use.
		{
			SQLite::Statement stmt{
				data->db.value(),
				"PRAGMA main.application_id;"
			};
			stmt.executeStep();
			app_id = stmt.getColumn(0);
		}

		if (app_id != application_id) {
			data->db.reset();
			return false;
		}

		return true;
	}
	void KVStore::close() {
		if (isOpen()) {
			data->batch.reset();

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

			resetStmts();
			data->db.reset();
		}
	}

	
	std::size_t KVStore::size() const {
		return numValues();
	}
	bool KVStore::empty() const {
		return size() == 0;
	}

	std::string KVStore::getKind() const {
		assert(isOpen());

		SQLite::Statement stmt{
			data->db.value(),
			"SELECT \"value\" FROM ez_kvstore_meta WHERE \"key\" = \"kind\";"
		};

		bool res = stmt.executeStep();
		assert(res);

		std::string kind;
		SQLite::Column col = stmt.getColumn(0);
		kind.assign((const char*)col.getBlob(), col.getBytes());
		return kind;
	}
	void KVStore::setKind(std::string_view kind) {
		assert(isOpen());

		SQLite::Statement stmt{
			data->db.value(),
			"INSERT INTO ez_kvstore_meta(\"key\", \"value\") VALUES (\"kind\", ?) ON CONFLICT(\"key\") DO UPDATE SET \"value\"=excluded.\"value\";"
		};
		stmt.bind(1, kind.data(), kind.length());

		stmt.executeStep();
	}

	bool KVStore::inBatch() const {
		return bool(data->batch);
	}
	bool KVStore::beginBatch() {
		if (!isOpen() || inBatch()) {
			return false;
		}

		data->batch.emplace(data->db.value());

		return true;
	}
	void KVStore::commitBatch() {
		if (!inBatch()) {
			throw std::logic_error("Attempt to commit a batch when not in a batch!");
		}
		data->batch.value().commit();
		data->batch.reset();
	}
	void KVStore::cancelBatch() {
		data->batch.reset();
	}



	bool KVStore::get(std::string_view name, std::string& data) const {
		const void* ptr;
		std::size_t len;
		if (getRaw(name, ptr, len)) {
			data.assign((const char*)ptr, len);
			return true;
		}
		return false;
	}
	bool KVStore::getView(std::string_view name, std::string_view& data) const {
		const void* ptr;
		std::size_t len;
		if (getRaw(name, ptr, len)) {
			data = std::string_view((const char*)ptr, len);
			return true;
		}
		return false;
	}
	bool KVStore::getStream(std::string_view name, ez::imemstream& stream) const {
		const void* ptr;
		std::size_t len;
		if (getRaw(name, ptr, len)) {
			stream.reset((const char*)ptr, len);
			return true;
		}
		return false;
	}

	bool KVStore::set(std::string_view name, std::string_view data) {
		return setRaw(name, (const void*)data.data(), data.length());
	}

	using const_iterator = KVStore::const_iterator;
	const_iterator KVStore::begin() const {
		return const_iterator(KVEntryViewGenerator(data->db.value(), "main"));
	}
	const_iterator KVStore::end() const {
		return const_iterator();
	}

	std::vector<KVEntry> KVStore::getEntries() const {
		std::vector<KVEntry> result;
		result.reserve(size());

		for (const KVEntryView& entry : *this) {
			result.push_back(KVEntry{ std::string(entry.key), std::string(entry.value) });
		}

		return result;
	}
	std::unordered_map<std::string, std::string> KVStore::getMap() const {
		std::unordered_map<std::string, std::string> result;
		result.reserve(size());

		for (const KVEntryView& entry : *this) {
			result.insert(std::make_pair(std::string(entry.key), std::string(entry.value)));
		}

		return result;
	}

	void KVStore::resetStmts() {
		data->containsStmt.reset();
		data->getStmt.reset();
		data->setStmt.reset();
		data->eraseStmt.reset();
		data->countStmt.reset();
	}
	void KVStore::createTable() {
		SQLite::Statement stmt(
			data->db.value(),
			"CREATE TABLE \"main\"("
			"\"hash\" INTEGER UNIQUE, "
			"\"key\" BLOB NOT NULL, "
			"\"value\" BLOB NOT NULL, "
			"PRIMARY KEY(\"hash\"));"
		);

		stmt.executeStep();

		resetStmts();
	}
}