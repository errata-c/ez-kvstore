#pragma once
#include <string_view>
#include <filesystem>
#include <cinttypes>
#include <iterator>
#include <ez/memstream.hpp>

namespace ez {
	int64_t kvhash(const char* data, std::size_t len);
	int64_t kvhash(std::string_view data);

	class KVPrivate;
	struct ElementIterator;
	struct TableIterator;

	class KVStore {
	public:
		struct const_iterator;
		struct const_table_iterator;

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


		bool containsTable(std::string_view name) const;
		bool getTable(std::string& name) const;
		bool createTable(std::string_view name);
		bool setTable(std::string_view name);
		bool setDefaultTable(std::string_view name);
		bool getDefaultTable(std::string& name) const;
		bool eraseTable(std::string_view name);
		bool renameTable(std::string_view old, std::string_view name);


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

		const_iterator begin() const;
		const_iterator end() const;

		const_table_iterator beginTables() const;
		const_table_iterator endTables() const;
	private:
		KVPrivate* impl;


	public:
		struct const_iterator {
			using iterator_category = std::input_iterator_tag;
			struct value_type {
				std::string key;
				std::string value;
			};
			using reference = value_type;
			using pointer = value_type;

			const_iterator();
			const_iterator(ElementIterator*);
			const_iterator(const_iterator&&) noexcept;
			const_iterator& operator=(const_iterator&&) noexcept;
			~const_iterator();

			bool operator==(const const_iterator& it) const;
			bool operator!=(const const_iterator& it) const;

			pointer operator->();
			reference operator*();

			const_iterator& operator++();
			void operator++(int);

			ElementIterator* iter;
		};
		struct const_table_iterator {
			using iterator_category = std::input_iterator_tag;
			using value_type = std::string;
			using reference = value_type;
			using pointer = value_type;

			const_table_iterator();
			const_table_iterator(const_table_iterator&&) noexcept;
			const_table_iterator& operator=(const_table_iterator&&) noexcept;
			const_table_iterator(TableIterator*);
			~const_table_iterator();

			bool operator==(const const_table_iterator& it) const;
			bool operator!=(const const_table_iterator& it) const;

			pointer operator->();
			reference operator*();

			const_table_iterator& operator++();
			void operator++(int);

			TableIterator* iter;
		};
	};
}