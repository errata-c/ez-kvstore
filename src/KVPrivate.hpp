#pragma once
#include <ez/KVStore.hpp>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>

namespace ez {
	class KVPrivate {
	public:
		KVPrivate();

		bool isOpen() const noexcept;
		bool create(const std::filesystem::path& path, bool overwrite);
		bool open(const std::filesystem::path& path, bool readonly);
		void close();

		bool empty() const noexcept;
		std::size_t numValues() const;
		std::size_t numTables() const;

		bool getKind(std::string & kind) const;
		bool setKind(std::string_view kind);

		bool containsTable(std::string_view name) const;
		bool getTable(std::string& name) const;
		bool createTable(std::string_view name);
		bool setTable(std::string_view name);
		bool setDefaultTable(std::string_view name);
		bool getDefaultTable(std::string& name) const;
		bool eraseTable(std::string_view name);

		bool inBatch() const;
		bool beginBatch();
		void commitBatch();
		void cancelBatch();

		bool contains(std::string_view name) const;

		bool get(std::string_view name, std::string& data) const;
		bool getRaw(std::string_view name, const void*& data, std::size_t& len) const;
		bool getStream(std::string_view name, ez::imemstream& stream) const;

		bool set(std::string_view name, std::string_view data);

		bool erase(std::string_view name);
	private:
		// No choice here, if I want to create the statements via lazy init, I need mutable.
		// Though the question remains, do I need to cache each of these statements?
		// I don't actually know if that makes much of a difference in terms of performance.
		mutable std::optional<SQLite::Database> db;
		mutable std::optional<SQLite::Transaction> batch;
		mutable std::optional<SQLite::Statement>
			containsStmt,
			getStmt,
			setStmt,
			eraseStmt,
			countStmt;

		std::string currentTable, tableID;
	};
}