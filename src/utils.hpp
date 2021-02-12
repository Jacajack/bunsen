#pragma once

#include <string>
#include <vector>

namespace bu {

template <typename T>
size_t vector_size(const std::vector<T> &v)
{
	return v.size() * sizeof(T);
}

std::string slurp_txt(const std::string &path);

}