#pragma once 

namespace bu {

struct no_move
{
	no_move(const no_move &) = default;
	no_move &operator=(const no_move &) = default;

	no_move(no_move &&) = delete;
	no_move &operator=(no_move &&) = delete;

	no_move() = default;
	~no_move() = default;

};

struct no_copy
{
	no_copy(const no_copy &) = delete;
	no_copy &operator=(const no_copy &) = delete;

	no_copy(no_copy &&) = default;
	no_copy &operator=(no_copy &&) = default;

	no_copy() = default;
	~no_copy() = default;
};

}