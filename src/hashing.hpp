#pragma once
#include <cinttypes>
#include <cstddef>
#include <string_view>

int64_t kvhash(const char* data, std::size_t len);
int64_t kvhash(std::string_view data);

int64_t kvhash(const char* data, std::size_t len, int64_t seed);
int64_t kvhash(std::string_view data, int64_t seed);