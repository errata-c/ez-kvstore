#include <ez/KVStore.hpp>
#include "KVPrivate.hpp"

#include "xxhash.h"

namespace ez {
	int64_t kvhash(const char* data, std::size_t len) {
		union {
			int64_t ival;
			uint64_t uval;
		} convert;

		convert.uval = XXH3_64bits(data, len);
		return convert.ival;
	}
	int64_t kvhash(std::string_view data) {
		return kvhash(data.data(), data.length());
	}

	KVStore::KVStore()
		: impl(new KVPrivate{})
	{}
	KVStore::~KVStore()
	{
		delete impl;
		impl = nullptr;
	}

	KVStore::KVStore(KVStore&& other) noexcept
		: impl(other.impl)
	{
		other.impl = nullptr;
	}
	KVStore& KVStore::operator=(KVStore&& other) noexcept {
		KVStore tmp{ std::move(other) };
		swap(tmp);
		return *this;
	}
	void KVStore::swap(KVStore& other) noexcept {
		std::swap(impl, other.impl);
	}

	bool KVStore::isOpen() const noexcept {
		return impl->isOpen();
	}

	bool KVStore::create(const std::filesystem::path& path, bool overwrite) {
		return impl->create(path, overwrite);
	}
	bool KVStore::open(const std::filesystem::path& path, bool readonly) {
		return impl->open(path, readonly);
	}
	void KVStore::close() {
		impl->close();
	}

	bool KVStore::empty() const noexcept {
		return impl->empty();
	}
	std::size_t KVStore::size() const noexcept {
		return impl->size();
	}

	bool KVStore::getKind(std::string& kind) const {
		return impl->getKind(kind);
	}
	bool KVStore::setKind(std::string_view kind) {
		return impl->setKind(kind);
	}

	bool KVStore::inBatch() const {
		return impl->inBatch();
	}
	bool KVStore::beginBatch() {
		return impl->beginBatch();
	}
	void KVStore::commitBatch() {
		impl->commitBatch();
	}
	void KVStore::cancelBatch() {
		impl->cancelBatch();
	}

	bool KVStore::contains(std::string_view name) const {
		return impl->contains(name);
	}

	bool KVStore::get(std::string_view name, std::string& data) const {
		return impl->get(name, data);
	}
	bool KVStore::getRaw(std::string_view name, const void*& data, std::size_t& len) const {
		return impl->getRaw(name, data, len);
	}
	bool KVStore::getStream(std::string_view name, ez::imemstream& stream) const {
		return impl->getStream(name, stream);
	}
	bool KVStore::set(std::string_view name, std::string_view data) {
		return impl->set(name, data);
	}
	bool KVStore::erase(std::string_view name) {
		return impl->erase(name);
	}
}