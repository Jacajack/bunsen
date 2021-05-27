#pragma once
#include <future>
#include <type_traits>
#include <list>
#include <queue>
#include "log.hpp"

namespace bu {

class async_task_cleaner;

class async_stop_flag
{
public:
	async_stop_flag() = default;
	async_stop_flag(async_stop_flag &&src) = default;
	async_stop_flag &operator=(async_stop_flag &&) = default;

	bool should_stop() const
	{
		return !m_active;
	}

	bool active() const
	{
		return m_active;
	}

	void request_stop()
	{
		m_active = false;
	}

protected:
	std::atomic<bool> m_active = true;
};

template <typename T>
class async_task : public std::future<T>
{
	template <typename Tpolicy, typename F, typename... Args>
	friend auto make_async_task(Tpolicy policy, F &&f, Args&&... args);

	template <typename Tpolicy, typename F, typename... Args>
	friend auto make_async_task(async_task_cleaner &cleaner, Tpolicy policy, F &&f, Args&&... args);

public:
	async_task() = default;
	async_task(const async_task &src) = delete;
	async_task(async_task &&src) noexcept = default;

	async_task &operator=(const async_task &) = delete;
	async_task &operator=(async_task &&rhs) noexcept = default;

	~async_task()
	{
		if (this->valid())
		{
			if (!m_cleaner)
			{
				LOG_WARNING << "~async_task() blocks!";
				wait();
			}
			else if (!is_ready_or_invalid())
			{
				LOG_DEBUG << "~async_task() self-discard";
				discard_task(*m_cleaner, std::move(*this));
			}
		}
	}

	void wait()
	{
		// LOG_INFO << "waiting for flag " << m_flag_ptr.get();
		if (std::future<T>::valid())
			std::future<T>::wait();
		m_flag_ptr.reset();
	}

	void request_stop()
	{
		if (m_flag_ptr)
			m_flag_ptr->request_stop();
	}

	bool is_ready() const
	{
		if (this->valid())
			return this->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
		else
			return false;
	}

	bool is_ready_or_invalid() const
	{
		return !this->valid() || !m_flag_ptr || is_ready();
	}

private:
	async_task &operator=(std::future<T> &&f)
	{
		std::future<T>::operator=(std::move(f));
		return *this;
	}

	async_task_cleaner *m_cleaner = nullptr;
	std::unique_ptr<bu::async_stop_flag> m_flag_ptr = std::make_unique<bu::async_stop_flag>();
};

template <typename Tpolicy, typename F, typename... Args>
auto make_async_task(Tpolicy policy, F &&f, Args&&... args)
{
	static_assert(std::is_invocable_v<F, const async_stop_flag*, Args...>, "Invalid arguments");
	async_task<std::invoke_result_t<std::decay_t<F>, const async_stop_flag*, std::decay_t<Args>...>> t;
	t = std::async(policy, f, t.m_flag_ptr.get(), args...);
	return t;
}

template <typename Tpolicy, typename F, typename... Args>
auto make_async_task(async_task_cleaner &cleaner, Tpolicy policy, F &&f, Args&&... args)
{
	auto t{make_async_task(policy, f, args...)};
	t.m_cleaner = &cleaner;
	return t;
}

template <typename T>
class async_task_collector
{
public:
	static bool empty()
	{
		return tasks.empty();
	}

	static void submit(async_task<T> &&t)
	{
		std::lock_guard l{mutex};
		t.request_stop();
		tasks.emplace_back(std::move(t));
	}

	static void collect()
	{
		// LOG_WARNING << "async_task_collector() collecting task!";

		// Pick one pending task to wait for
		async_task<T> *tptr = nullptr;
		{
			std::lock_guard l{mutex};
			for (auto &t : tasks)
				if (!t.is_ready_or_invalid())
					tptr = &t;
		}

		if (tptr) tptr->wait();

		// Erase dead tasks
		{
			std::lock_guard l{mutex};
			for (auto it = tasks.begin(); it != tasks.end(); )
			{
				if (it->is_ready_or_invalid())
					it = tasks.erase(it);
				else
					++it;
			}
		}
	}

private:
	inline static std::mutex mutex;
	inline static std::list<async_task<T>> tasks;
};

class async_task_cleaner
{
public:
	void submit(std::function<void(void)> f)
	{
		std::lock_guard l{m_mutex};
		m_call_queue.push(f);
	}

	bool empty() const
	{
		return m_call_queue.empty();
	}

	bool collect()
	{
		std::function<void(void)> f;
		{
			std::lock_guard l{m_mutex};
			if (m_call_queue.empty())
				return false;
			else
			{
				f = m_call_queue.front();
				m_call_queue.pop();
			}
		}
		f();
		return true;
	}


private:
	std::queue<std::function<void(void)>> m_call_queue;
	std::mutex m_mutex;
};

template <typename T>
void discard_task(async_task_cleaner &cl, async_task<T> &&t)
{
	LOG_DEBUG << "Discarding task";
	async_task_collector<T>::submit(std::move(t));
	cl.submit(async_task_collector<T>::collect);
}

inline async_task_cleaner global_task_cleaner;

}

