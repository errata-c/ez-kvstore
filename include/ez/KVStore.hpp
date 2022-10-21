#pragma once
#include <string_view>
#include <filesystem>
#include <cinttypes>
#include <iterator>
#include <vector>
#include <unordered_map>
#include <ez/memstream.hpp>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>
#include <ez/intern/KVEntry.hpp>
#include <ez/intern/KVIterator.hpp>
#include <ez/intern/KVGenerators.hpp>

namespace ez {
	/*
	* Simple key-value file format. Built on top of sqlite3.
	* Only has a single table, for simplicity.
	*/
	class KVStore {
	public:
		using const_iterator = KVIterator<KVEntryViewGenerator, KVEntryView>;
		using iterator = const_iterator;

		KVStore();

		~KVStore() = default;
		KVStore(KVStore&&) noexcept = default;
		KVStore& operator=(KVStore&&) noexcept = default;

		//KVStore(const KVStore &);
		//KVStore& operator=(const KVStore&);
		
		void swap(KVStore& other) noexcept;

		bool isOpen() const noexcept;

		bool create(const std::filesystem::path& path, bool overwrite = false);
		bool open(const std::filesystem::path & path, bool readonly = false);
		void close();
		
		// Return the number of values in the current table.
		std::size_t numValues() const;
		std::size_t size() const;
		bool empty() const;

		// Return the string identifying the kind of key store.
		std::string getKind() const;
		// Set the string to identifiy the kind of key store.
		void setKind(std::string_view kind);

		// Returns true if the current table contains a value with the key.
		bool contains(std::string_view name) const;

		// Should probably implement a error messaging system to make this better.
		// Return an error object that can be queryed for info about what went wrong.
		bool get(std::string_view name, std::string & data) const;
		bool getView(std::string_view name, std::string_view& data) const;
		bool getRaw(std::string_view name, const void*& data, std::size_t& len) const;
		bool getStream(std::string_view name, ez::imemstream & stream) const;

		bool set(std::string_view name, std::string_view data);
		bool setRaw(std::string_view name, const void* data, std::size_t len);

		bool erase(std::string_view name);

		bool rename(std::string_view old, std::string_view name);

		void clear();

		bool inBatch() const;
		bool beginBatch();
		void commitBatch();
		void cancelBatch();

		const_iterator begin() const;
		const_iterator end() const;

		std::vector<KVEntry> getEntries() const;
		std::unordered_map<std::string, std::string> getMap() const;
	private:
		void resetStmts();
		void createTable();

		struct Data {
			// Mutable is necessary for lazy initialization.
			mutable std::optional<SQLite::Database> db;
			mutable std::optional<SQLite::Transaction> batch;
			mutable std::optional<SQLite::Statement>
				containsStmt,
				getStmt,
				setStmt,
				eraseStmt,
				countStmt;
		};
		mutable std::unique_ptr<Data> data;
	};
}