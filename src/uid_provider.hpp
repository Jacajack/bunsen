#pragma once
#include <cstdint>

namespace bu {

template <typename T>
class uid_provider
{
public:
	using id_type = std::uint64_t;

	uid_provider() : 
		m_uid(m_counter ++ )
	{
	}

	uid_provider(uid_provider &&src) noexcept
	{
		m_uid = src.m_uid;
		src.m_uid = 0;
	}
	
	~uid_provider() = default;

	uid_provider &operator=(uid_provider &&src) noexcept
	{
		if (&src == this) return *this;
		m_uid = src.m_uid;
		src.m_uid = 0;
		return *this;
	}

	bool operator==(const uid_provider &src) noexcept
	{
		return (m_uid && src.m_uid) && m_uid == src.m_uid;
	}

	bool operator!=(const uid_provider &src) noexcept
	{
		return !operator==(src);
	}

	const id_type &uid() const
	{
		return m_uid;
	}

	uid_provider(const uid_provider&) = delete; 
	uid_provider &operator=(const uid_provider&) = delete; 

private:
	id_type m_uid;

	static id_type m_counter;
};

template <typename T>
typename uid_provider<T>::id_type uid_provider<T>::m_counter = 1;

}