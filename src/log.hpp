#pragma once

#include <iostream>
#include <iomanip>
#include <chrono>

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
	log_stream(std::ostream &output, const std::string &prefix, int color) : 
		std::ostream(this),
		m_output(output)
	{
		auto duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time);
		double s = duration.count();
		m_output << "\x1b[92m[" << std::fixed << std::setprecision(5) << s << "]\x1b[0m ";
		m_output << "\x1b[3" << color << "m" << prefix << "\x1b[0m";
	}

	~log_stream()
	{
		m_output << "\x1b[0m" << std::endl;
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
};

struct log_error : public log_stream
{
	log_error() :
		log_stream(std::cerr, "error: ", 1)
	{}
};

struct log_warning : public log_stream
{
	log_warning() :
		log_stream(std::cerr, "warning: ", 3)
	{}
};

struct log_info : public log_stream
{
	log_info() :
		log_stream(std::cerr, "info: ", 4)
	{}
};

struct log_debug : public log_stream
{
	log_debug() :
		log_stream(std::cerr, "debug: ", 5)
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
#define LOG_DEBUG bu::log_debug{} << __FILE__ << ":" << __LINE__ << ": "
#else
#define LOG_DEBUG bu::dummy_stream{}
#endif