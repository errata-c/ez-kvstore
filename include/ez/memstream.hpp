#pragma once
#include <streambuf>

namespace ez {
	class membuf : public std::streambuf {
	public:
		membuf()
		{
			this->setg(0, 0, 0);
		}
		template<typename T>
		membuf(const T* first, const T* last)
		{
			this->setg((char*)const_cast<T*>(first), (char*)const_cast<T*>(first), (char*)const_cast<T*>(last));
		}
		template<typename T>
		membuf(const T* first, std::size_t num)
		{
			this->setg((char*)const_cast<T*>(first), (char*)const_cast<T*>(first), (char*)const_cast<T*>(first + num));
		}

		template<typename T>
		void reset(const T* first, const T* last) {
			this->setg((char*)const_cast<T*>(first), (char*)const_cast<T*>(first), (char*)const_cast<T*>(last));
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

		template<typename T>
		imemstream(const T* data, std::size_t size)
			: std::istream(&mbuf)
			, mbuf(data, size)
		{}
		template<typename T>
		imemstream(const T* data, const T* end)
			: std::istream(&mbuf)
			, mbuf(data, end)
		{}

		imemstream(const char * data, std::size_t size)
			: std::istream(&mbuf)
			, mbuf(data, size)
		{}
		imemstream(const char* data, const char* end)
			: std::istream(&mbuf)
			, mbuf(data, end)
		{}

		template<typename T>
		void reset(const T* first, const T* last) {
			mbuf.reset(first, last);
		}
		void reset() {
			mbuf.reset();
		}
	private:
		membuf mbuf;
	};
}