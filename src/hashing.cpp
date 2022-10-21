#include "hashing.hpp"

#include <xxhash.h>

int64_t kvhash(const char* data, std::size_t len) {
	return kvhash(data, len, 0);
}
int64_t kvhash(std::string_view text) {
	return kvhash(text.data(), text.length(), 0);
}

int64_t kvhash(const char* data, std::size_t len, int64_t seed) {
	return static_cast<int64_t>(XXH64(data, len, static_cast<uint64_t>(seed)));
}
int64_t kvhash(std::string_view text, int64_t seed) {
	return kvhash(text.data(), text.length(), seed);
}