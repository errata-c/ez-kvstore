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


	std::size_t KVStore::numValues() const noexcept {
		return impl->numValues();
	}
	std::size_t KVStore::numTables() const noexcept {
		return impl->numTables();
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



	bool KVStore::containsTable(std::string_view name) const {
		return impl->containsTable(name);
	}
	bool KVStore::getTable(std::string& name) const {
		return impl->getTable(name);
	}
	bool KVStore::createTable(std::string_view name) {
		return impl->createTable(name);
	}
	bool KVStore::setTable(std::string_view name) {
		return impl->setTable(name);
	}
	bool KVStore::setDefaultTable(std::string_view name) {
		return impl->setDefaultTable(name);
	}
	bool KVStore::getDefaultTable(std::string& name) const {
		return impl->getDefaultTable(name);
	}
	bool KVStore::eraseTable(std::string_view name) {
		return impl->eraseTable(name);
	}
	bool KVStore::renameTable(std::string_view old, std::string_view name) {
		return impl->renameTable(old, name);
	}



	bool KVStore::contains(std::string_view name) const {
		return impl->contains(name);
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
	bool KVStore::getRaw(std::string_view name, const void*& data, std::size_t& len) const {
		return impl->getRaw(name, data, len);
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
	bool KVStore::setRaw(std::string_view name, const void* data, std::size_t len) {
		return impl->setRaw(name, data, len);
	}

	bool KVStore::erase(std::string_view name) {
		return impl->erase(name);
	}
	bool KVStore::rename(std::string_view old, std::string_view name) {
		return impl->rename(old, name);
	}
	void KVStore::clear() {
		impl->clear();
	}


	using const_iterator = KVStore::const_iterator;
	using const_table_iterator = KVStore::const_table_iterator;


	const_iterator KVStore::begin() const {
		return impl->elementIterator();
	}
	const_iterator KVStore::end() const {
		return {};
	}

	const_table_iterator KVStore::beginTables() const {
		return impl->tableIterator();
	}
	const_table_iterator KVStore::endTables() const {
		return {};
	}


	const_iterator::const_iterator()
		: iter(nullptr)
	{}
	const_iterator::const_iterator(const_iterator&& other) noexcept
		: iter(other.iter)
	{
		other.iter = nullptr;
	}
	const_iterator& const_iterator::operator=(const_iterator&& other) noexcept {
		delete iter;
		iter = other.iter;
		other.iter = nullptr;

		return *this;
	}
	const_iterator::const_iterator(ElementIterator* ptr)
		: iter(ptr)
	{}
	const_iterator::~const_iterator() {
		delete iter;
		iter = nullptr;
	}

	bool const_iterator::operator==(const const_iterator& other) const {
		return iter == nullptr || iter->atEnd();
	}
	bool const_iterator::operator!=(const const_iterator& other) const {
		return iter != nullptr && !iter->atEnd();
	}

	const_iterator::pointer const_iterator::operator->() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		return {iter->key(), iter->value()};
	}
	const_iterator::reference const_iterator::operator*() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		return { iter->key(), iter->value() };
	}

	const_iterator& const_iterator::operator++() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		iter->advance();

		return *this;
	}
	void const_iterator::operator++(int) {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		iter->advance();
	}




	const_table_iterator::const_table_iterator()
		: iter(nullptr)
	{}
	const_table_iterator::const_table_iterator(const_table_iterator&& other) noexcept
		: iter(other.iter)
	{
		other.iter = nullptr;
	}
	const_table_iterator& const_table_iterator::operator=(const_table_iterator&& other) noexcept {
		delete iter;
		iter = other.iter;
		other.iter = nullptr;

		return *this;
	}
	const_table_iterator::const_table_iterator(TableIterator* ptr)
		: iter(ptr)
	{}
	const_table_iterator::~const_table_iterator() {
		delete iter;
		iter = nullptr;
	}

	bool const_table_iterator::operator==(const const_table_iterator& it) const {
		return iter == nullptr || iter->atEnd();
	}
	bool const_table_iterator::operator!=(const const_table_iterator& it) const {
		return iter != nullptr && !iter->atEnd();
	}

	const_table_iterator::pointer const_table_iterator::operator->() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		return iter->name();
	}
	const_table_iterator::reference const_table_iterator::operator*() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		return iter->name();
	}

	const_table_iterator& const_table_iterator::operator++() {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		iter->advance();

		return *this;
	}
	void const_table_iterator::operator++(int) {
		assert(iter != nullptr);
		assert(!iter->atEnd());

		iter->advance();
	}
}