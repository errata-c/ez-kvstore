#pragma once
#include <string_view>
#include <filesystem>
#include <cinttypes>
#include <ez/memstream.hpp>

namespace ez {
	int64_t kvhash(const char* data, std::size_t len);
	int64_t kvhash(std::string_view data);

	class KVPrivate;

	class KVStore {
	public:
		KVStore();
		~KVStore();

		//KVStore(const KVStore &);
		KVStore(KVStore&&) noexcept;

		//KVStore& operator=(const KVStore&);
		KVStore& operator=(KVStore&&) noexcept;
		void swap(KVStore& other) noexcept;

		bool isOpen() const noexcept;

		bool create(const std::filesystem::path& path, bool overwrite = false);
		bool open(const std::filesystem::path & path, bool readonly = false);
		void close();
		
		
		// Return the number of values in the current table.
		std::size_t numValues() const noexcept;
		// Return the number of tables in the current database.
		std::size_t numTables() const noexcept;

		// Return the string identifying the kind of key store.
		bool getKind(std::string& kind) const;
		// Set the string to identifiy the kind of key store.
		bool setKind(std::string_view kind);


		// Returns true when currently in a batch operation.
		bool inBatch() const;

		// Attempt to start a batch of writes to the key store.
		bool beginBatch();

		// Commit the batch of writes to the key store.
		void commitBatch();

		// Cancel a batch of writes to the key store, and rollback the changes.
		void cancelBatch();

		// Returns true if the current table contains a value with the key.
		bool contains(std::string_view name) const;

		// Should probably implement a error messaging system to make this better.
		// Return an error object that can be queryed for info about what went wrong.
		bool get(std::string_view name, std::string & data) const;
		bool getRaw(std::string_view name, const void*& data, std::size_t& len) const;
		bool getStream(std::string_view name, ez::imemstream & stream) const;

		bool set(std::string_view name, std::string_view data);
		bool setRaw(std::string_view name, const void* data, std::size_t len);

		bool erase(std::string_view name);
	private:
		KVPrivate* impl;
	};
}