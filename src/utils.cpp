#include "utils.hpp"
#include <sstream>
#include <fstream>
#include <exception>

using namespace std::string_literals;

std::string br::slurp_txt(const std::string &path)
{
	std::ifstream f(path);
	if (!f) throw std::runtime_error("could not read file '"s + path + "'"s);
	std::stringstream buf;
	buf << f.rdbuf();
	return buf.str();
}