#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T>
class ThreadSafeQueue
{
public:
    virtual ~ThreadSafeQueue() { clear(); }

public:
    const T& front() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.front();
    }

    const T& back() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.back();
    }

    bool empty() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.empty();
    }

    size_t size() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.size();
    }

    void pushBack(const T& item)
    {
        std::scoped_lock lock(m_mutex);
        m_deque.emplace_back(std::move(item));

        resume();
    }

    T popBack()
    {
        if (empty()) wait();
        std::scoped_lock lock(m_mutex);
        auto item{ std::move(m_deque.back()) };
        m_deque.pop_back();

        return item;
    }

    void pushFront(const T& item)
    {
        std::scoped_lock lock(m_mutex);
        m_deque.emplace_front(std::move(item));

        resume();
    }

    T popFront()
    {
        if (empty()) wait();
        std::scoped_lock lock(m_mutex);
        auto item{ std::move(m_deque.front()) };
        m_deque.pop_front();

        return item;
    }

    void clear()
    {
        std::scoped_lock lock(m_mutex);
        m_deque.clear();
    }

    void wait() const
    {
        std::unique_lock<std::mutex> lock(m_blockingMutex);
        m_condition.wait(lock);
    }

    void resume() const
    {
        std::unique_lock<std::mutex> lock(m_blockingMutex);
        m_condition.notify_one();
    }

protected:
    mutable std::mutex m_mutex;
    mutable std::mutex m_blockingMutex;
    mutable std::condition_variable m_condition;
    std::deque<T> m_deque;
};

