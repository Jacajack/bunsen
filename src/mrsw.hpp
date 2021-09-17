#pragma once
#include <mutex>
#include <condition_variable>
#include "no_copy.hpp"

namespace bu {

template <typename T>
class mrsw;

template <typename T>
class mrsw_reader : public bu::no_copy
{
	friend class mrsw<T>;

public:
	mrsw_reader(mrsw_reader &&src) :
		m_ptr(std::exchange(src.m_ptr, nullptr))
	{
	}

	mrsw_reader &operator=(mrsw_reader &&rhs) noexcept
	{
		if (this == &rhs) return *this;
		m_ptr = std::exchange(rhs.m_ptr, nullptr);
		return *this;
	}

	~mrsw_reader()
	{
		if (m_ptr)
			m_ptr->unregister_reader();
	}

	const T &cref() const {return m_ptr->cref();}
	const T &operator*() const {return cref();}
	const T *operator->() const {return &cref();}

private:
	explicit mrsw_reader(const mrsw<T> *ptr) :
		m_ptr(ptr)
	{
		m_ptr->register_reader();
	}

	const mrsw<T> *m_ptr;
};

template <typename T>
class mrsw_writer : public bu::no_copy
{
	friend class mrsw<T>;

public:
	mrsw_writer(mrsw_writer &&src) :
		m_ptr(std::exchange(src.m_ptr, nullptr))
	{
	}

	mrsw_writer &operator=(mrsw_writer &&rhs) noexcept
	{
		if (this == &rhs) return *this;
		m_ptr = std::exchange(rhs.m_ptr, nullptr);
		return *this;
	}

	~mrsw_writer()
	{
		if (m_ptr)
			m_ptr->unregister_writer();
	}

	const T &ref() {return m_ptr->ref();}
	const T &operator*() {return ref();}
	const T *operator->() {return &ref();}

private:
	explicit mrsw_writer(mrsw<T> *ptr) :
		m_ptr(ptr)
	{
		m_ptr->register_writer();
	}

	mrsw<T> *m_ptr;
};

template <typename T>
class mrsw
{
	friend class mrsw_reader<T>;
	friend class mrsw_writer<T>;

public:
	template <typename... Args>
	mrsw(Args... args) :
		m_data(args...)
	{
	}

	mrsw_reader<T> r() const
	{
		return mrsw_reader{this};
	}

	mrsw_writer<T> w()
	{
		return mrsw_writer{this};
	}

private:
	T &ref() {return m_data;}
	const T &cref() const {return m_data;}

	void register_reader() const
	{
		std::unique_lock<std::mutex> lock{m_mutex};
		while (m_waiting_writers)
			m_reader_cv.wait(lock);
		m_readers++;
		lock.unlock();
	}

	void unregister_reader() const
	{
		std::unique_lock<std::mutex> lock{m_mutex};
		m_readers--;
		lock.unlock();
		m_writer_cv.notify_one();
	}

	void register_writer()
	{
		std::unique_lock<std::mutex> lock{m_mutex};
		m_waiting_writers++;
		while (m_readers || m_writers)
			m_writer_cv.wait(lock);
		m_writers++;
		lock.unlock();
	}

	void unregister_writer()
	{
		std::unique_lock<std::mutex> lock{m_mutex};
		m_waiting_writers--;
		m_writers--;
		if (m_waiting_writers)
			m_writer_cv.notify_one();
		else
			m_reader_cv.notify_all();
		lock.unlock();
	}

	mutable std::mutex m_mutex;
	mutable std::condition_variable m_reader_cv;
	mutable std::condition_variable m_writer_cv;
	mutable int m_readers = 0;
	mutable int m_writers = 0;
	mutable int m_waiting_writers = 0;

	T m_data;
};



}