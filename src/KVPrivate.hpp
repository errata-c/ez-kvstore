#pragma once
#include <ez/KVStore.hpp>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>

namespace ez {
	class KVPrivate {
	public:
		bool isOpen() const noexcept;
		bool create(const std::filesystem::path& path, bool overwrite);
		bool open(const std::filesystem::path& path, bool readonly);
		void close();

		bool empty() const noexcept;
		std::size_t size() const noexcept;

		bool getKind(std::string & kind);
		bool setKind(std::string_view kind);

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
		mutable std::optional<SQLite::Database> db;
		mutable std::optional<SQLite::Transaction> batch;
		mutable std::optional<SQLite::Statement>
			containsStmt,
			getStmt,
			setStmt,
			eraseStmt,
			countStmt;

		
	};
}