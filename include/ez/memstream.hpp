#pragma once
#include <streambuf>

namespace ez {
	class membuf : public std::streambuf {
	public:
		using char_type = char;
		using int_type = std::streambuf::int_type;
		using pos_type = std::streambuf::pos_type;
		using off_type = std::streambuf::off_type;

		membuf()
		{
			reset();
		}
		membuf(const char* first, const char* last)
		{
			reset(first, last);
		}
		membuf(const char* first, std::size_t len)
		{
			reset(first, len);
		}

		membuf(membuf&& other)
		{}
		membuf& operator=(membuf&& other) {
			membuf tmp{ std::move(other) };
			this->swap(tmp);

			return *this;
		}

		membuf(const membuf&) = delete;
		membuf& operator=(const membuf&) = delete;

		void reset(const char* first, std::size_t len) {
			reset(first, first + len);
		}
		void reset(const char* first, const char* last) {
			this->setg(const_cast<char*>(first), const_cast<char*>(first), const_cast<char*>(last));
		}
		void reset() {
			this->setg(0, 0, 0);
		}

		void swap(membuf & other) {
			std::streambuf::swap(other);
		}
	protected:
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
		{
			pos_type res = pos_type(off_type(-1));

			switch (dir) {
			case std::ios_base::cur:
				this->gbump(off);
				res = this->gptr() - this->eback();
				break;
			case std::ios_base::end:
				this->setg(this->eback(), this->egptr() + off, this->egptr());
				res = this->gptr() - this->eback();
				break;
			case std::ios_base::beg:
				this->setg(this->eback(), this->eback() + off, this->egptr());
				res = this->gptr() - this->eback();
				break;
			}

			return res;
		}

		pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
		{
			return this->seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
		}
	};

	class imemstream: public std::istream {
	public:
		imemstream()
			: std::istream(&mbuf)
			, mbuf()
		{}
		imemstream(const char * data, std::size_t size)
			: std::istream(&mbuf)
			, mbuf(data, size)
		{}
		imemstream(const char* data, const char* end)
			: std::istream(&mbuf)
			, mbuf(data, end)
		{}

		imemstream(imemstream&&) = default;
		imemstream& operator=(imemstream&&) = default;

		void reset(const char* first, const char* last) {
			mbuf.reset(first, last);
		}
		void reset(const char* first, std::size_t len) {
			mbuf.reset(first, first + len);
		}
		void reset() {
			mbuf.reset();
		}
	private:
		membuf mbuf;
	};
}