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
		
		bool empty() const noexcept;
		std::size_t size() const noexcept;

		bool getKind(std::string& kind) const;
		bool setKind(std::string_view kind);

		bool inBatch() const;
		bool beginBatch();
		void commitBatch();
		void cancelBatch();

		bool contains(std::string_view name) const;

		bool get(std::string_view name, std::string & data) const;
		bool getRaw(std::string_view name, const void*& data, std::size_t& len) const;
		bool getStream(std::string_view name, ez::imemstream & stream) const;
		bool set(std::string_view name, std::string_view data);
		bool erase(std::string_view name);
	private:
		KVPrivate* impl;
	};
}