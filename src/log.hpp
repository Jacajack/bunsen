#pragma once
#include <iostream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <optional>
#include "utils.hpp"

namespace bu {

struct dummy_stream
{
	template <typename T>
	const dummy_stream &operator<<(T&&) const noexcept
	{
		return *this;
	}
};

class log_stream : private std::streambuf, public std::ostream
{
public:
	log_stream(std::ostream &output, const std::string &prefix, int color, std::mutex *mutex = nullptr) : 
		std::ostream(this),
		m_output(output)
	{
		if (mutex)
			m_lock.emplace(*mutex);

		auto duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time);
		double s = duration.count();
		m_output << "\x1b[92m[" << std::fixed << std::setprecision(5) << s << "]\x1b[0m ";
		m_output << "\x1b[3" << color << "m" << prefix << "\x1b[0m";
	}

	~log_stream()
	{
		m_output << "\x1b[0m" << std::endl;
		m_lock.reset();
	}

	static std::chrono::time_point<std::chrono::steady_clock> start_time;

private:
	int overflow(int c)
	{
		output_char(c);
		return 0;
	}

	void output_char(int c)
	{
		m_output.put(c);
	}

	std::ostream &m_output;
	std::optional<std::lock_guard<std::mutex>> m_lock;
};

extern std::mutex log_cerr_mutex;

struct log_error : public log_stream
{
	log_error() :
		log_stream(std::cerr, "error: ", 1, &log_cerr_mutex)
	{}
};

struct log_warning : public log_stream
{
	log_warning() :
		log_stream(std::cerr, "warning: ", 3, &log_cerr_mutex)
	{}
};

struct log_info : public log_stream
{
	log_info() :
		log_stream(std::cerr, "info: ", 4, &log_cerr_mutex)
	{}
};

struct log_debug : public log_stream
{
	log_debug() :
		log_stream(std::cerr, "debug: ", 5, &log_cerr_mutex)
	{}
};

}

#define LOG_ERROR bu::log_error{}
#define LOG_WARNING bu::log_warning{}
#define LOG_INFO bu::log_info{}

// Make debug logging dependent on standard debug macro
#ifdef DEBUG
#define DEBUG_LOGGING
#endif
#ifdef NDEBUG
#undef DEBUG_LOGGING
#endif

// Debug logging is fully optional
#ifdef DEBUG_LOGGING
#define LOG_DEBUG bu::log_stream{std::cerr, "debug: " __FILE__ ":" BU_TO_STR(__LINE__) ": ", 5, &bu::log_cerr_mutex}
#else
#define LOG_DEBUG bu::dummy_stream{}
#endif