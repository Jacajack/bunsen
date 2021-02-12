#include "log.hpp"

// Initialize reference time
std::chrono::time_point<std::chrono::steady_clock> bu::log_stream::start_time = std::chrono::steady_clock::now();
