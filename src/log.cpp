#include "log.hpp"

// Initialize reference time
std::chrono::time_point<std::chrono::steady_clock> br::log_stream::start_time = std::chrono::steady_clock::now();
