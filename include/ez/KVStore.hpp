#pragma once
#include <string_view>
#include <filesystem>
#include <ez/memstream.hpp>

namespace ez {
	class KVPrivate;

	class KVStore {
	public:
		KVStore();
		~KVStore();

		//KVStore(const KVStore &);
		KVStore(KVStore&&) noexcept;

		//KVStore& operator=(const KVStore&);
		//KVStore& operator=(KVStore&&) noexcept;

		bool isOpen() const noexcept;

		bool create(const std::filesystem::path& path, bool overwrite = false);
		bool open(const std::filesystem::path & path, bool readonly = false);
		void close();
		
		bool empty() const noexcept;
		std::size_t size() const noexcept;

		bool inBatch() const;
		bool beginBatch();
		void commitBatch();
		void cancelBatch();

		bool contains(std::string_view name) const;

		bool get(std::string_view name, std::string & data) const;
		bool getStream(std::string_view name, ez::imemstream & stream) const;
		bool set(std::string_view name, std::string_view data);
		bool erase(std::string_view name);
	private:
		KVPrivate* impl;
	};
}