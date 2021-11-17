#pragma once
#include <streambuf>

namespace ez {
	class membuf : public std::streambuf {
	public:
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

		void reset(const char* first, std::size_t len) {
			reset(first, first + len);
		}
		void reset(const char* first, const char* last) {
			this->setg(const_cast<char*>(first), const_cast<char*>(first), const_cast<char*>(last));
		}
		void reset() {
			this->setg(0, 0, 0);
		}
	protected:
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
		{
			switch (dir) {
			case std::ios_base::cur:
				gbump(off);
				break;
			case std::ios_base::end:
				setg(eback(), egptr() + off, egptr());
				break;
			case std::ios_base::beg:
				setg(eback(), eback() + off, egptr());
				break;
			}

			return gptr() - eback();
		}

		pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
		{
			return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
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