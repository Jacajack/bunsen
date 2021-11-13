#pragma once

//! \todo split this into separate files

#include <string>
#include <vector>
#include <future>
#include <glm/glm.hpp>
#include <imgui.h>

namespace bu {

template <typename T>
size_t vector_size(const std::vector<T> &v)
{
	return v.size() * sizeof(T);
}

std::string slurp_txt(const std::string &path);

inline glm::vec2 to_vec2(const ImVec2 &v)
{
	return {v.x, v.y};
}

inline glm::vec4 to_vec4(const ImVec4 &v)
{
	return {v.x, v.y, v.z, v.w};
}

template <typename T>
inline bool is_future_ready(const std::future<T> &f)
{
	return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

}

#define BU_TO_STR_(x) #x
#define BU_TO_STR(x) BU_TO_STR_(x)
#define const_this (const_cast<const decltype(this)>(this))