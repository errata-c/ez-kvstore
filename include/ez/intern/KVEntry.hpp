#pragma once
#include <string>
#include <string_view>

namespace ez {
	struct KVEntry {
		std::string key, value;
	};
	struct KVEntryView {
		std::string_view key, value;
	};
}