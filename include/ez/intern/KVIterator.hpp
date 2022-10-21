#pragma once
#include <memory>
#include <iterator>
#include <type_traits>
#include <cassert>

namespace ez {
	/*
	Class to create a standard input iterator from a simple generator class.
	Expects a Generator class and the value it will be generating.
	Generator needs one method:
	bool advance(value_type & out);	
	*/
	template<typename Generator, typename Value>
	class KVIterator {
		struct Data {
			Generator generator;
			Value value;
		};
	public:
		using value_type = Value;
		using iterator_category = std::input_iterator_tag;
		using reference = value_type&;
		using pointer = value_type*;

		static_assert(std::is_default_constructible_v<value_type>, "ez::KVIterator requires a default constructible value_type!");

		KVIterator()
		{}
		KVIterator(Generator && _gen)
			: data(new Data{std::move(_gen)})
		{
			++(*this);
		}
		KVIterator(const Generator& _gen)
			: data(new Data{ _gen })
		{
			++(*this);
		}

		KVIterator(KVIterator&&) noexcept = default;
		KVIterator& operator=(KVIterator&&) noexcept = default;
		KVIterator(const KVIterator&) = default;
		KVIterator& operator=(const KVIterator&) = default;

		~KVIterator() = default;

		bool operator==(const KVIterator& it) const {
			return data.get() == it.data.get();
		}
		bool operator!=(const KVIterator& it) const {
			return data.get() != it.data.get();
		}

		pointer operator->() {
			assert(data);
			return &data->value;
		}
		reference operator*() {
			assert(data);
			return data->value;
		}

		KVIterator& operator++() {
			assert(data);
			if (!data->generator.advance(data->value)) {
				data.reset();
			}
			return *this;
		}
		void operator++(int) {
			++(*this);
		}
	private:
		std::shared_ptr<Data> data;
	};
}