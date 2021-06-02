#pragma once
#include <vector>

namespace bu::rt {

/**
	\brief Pool allocator meant for std::multiset used in bu::rt::minmax_tracker
	\warning This allocator doesn't free any memory until destruction!
	
	\note The improvement isn't *that* big but allowed to reduce find_split
	time from ~400ms to around 300ms.
*/
template <typename T, std::size_t Pool_size = 65536>
class pool_allocator
{
public:
	using value_type = T;
	using size_type = std::size_t;
	using type = T;

	template <typename U>
	struct rebind
	{
		using other = pool_allocator<U, Pool_size>;
	};

	pool_allocator()
	{
		m_pools.reserve(64);
	}

	~pool_allocator()
	{
		for (auto ptr : m_pools)
			operator delete(ptr);
	}

	T *allocate(std::size_t n)
	{
		// ZoneScopedN("pool_allocator<T>::allocate()");
		// TracyPlot("pool_space", (int64_t)m_pool_space);
		// TracyPlot("pool_count", (int64_t)m_pools.size());

		if (n > Pool_size)
		{
			// ZoneScopedN("pool_allocator<T> - custom pool");
			m_pools.push_back(static_cast<T*>(operator new(n * sizeof(T))));
			m_pool_space = 0;
			return m_pools.back();
		}
		else if (n > m_pool_space)
		{
			// ZoneScopedN("pool_allocator<T> - new pool");
			m_pools.push_back(static_cast<T*>(operator new(Pool_size * sizeof(T))));
			m_pool_space = Pool_size - n;
			return m_pools.back();
		}
		else
		{
			// ZoneScopedN("pool_allocator<T> - old");
			auto ptr = m_pools.back() + Pool_size - m_pool_space;
			m_pool_space -= n;
			return ptr;
		}
	}

	void deallocate(T *ptr, std::size_t n)
	{
	}

private:
	std::vector<T*> m_pools;
	std::size_t m_pool_space = 0;
};

}