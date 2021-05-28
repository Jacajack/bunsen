#pragma once

namespace bu::rt {

template <typename T, int Size>
class linear_stack
{
public:
	linear_stack() = default;

	void push(const T &t)
	{
		m_arr[m_size++] = t;
	}

	void pop()
	{
		--m_size;
	}

	const T &top() const
	{
		return m_arr[m_size - 1];
	}

	bool empty() const
	{
		return m_size == 0;
	}

private:
	T m_arr[Size];
	int m_size = 0;
};

}