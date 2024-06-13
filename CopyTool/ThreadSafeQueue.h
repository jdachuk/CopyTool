#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T, int MAX_SIZE=32>
class ThreadSafeQueue
{
public:
    virtual ~ThreadSafeQueue() { clear(); }

public:
    const T front() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.front();
    }

    const T back() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.back();
    }

    bool empty() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.empty();
    }

    bool full() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.size() >= MAX_SIZE;
    }

    size_t size() const
    {
        std::scoped_lock lock(m_mutex);
        return m_deque.size();
    }

    void pushBack(T&& item)
    {
        if (full()) waitWrite();
        std::scoped_lock lock(m_mutex);
        m_deque.emplace_back(std::move(item));

        resumeRead();
    }

    T popBack()
    {
        if (empty()) waitRead();
        std::scoped_lock lock(m_mutex);
        auto item{ std::move(m_deque.back()) };
        m_deque.pop_back();

        resumeWrite();

        return item;
    }

    void pushFront(T&& item)
    {
        if (full()) waitWrite();
        std::scoped_lock lock(m_mutex);
        m_deque.emplace_front(std::move(item));

        resumeRead();
    }

    T popFront()
    {
        if (empty()) waitRead();
        std::scoped_lock lock(m_mutex);
        auto item{ std::move(m_deque.front()) };
        m_deque.pop_front();

        resumeWrite();

        return item;
    }

    void clear()
    {
        std::scoped_lock lock(m_mutex);
        m_deque.clear();
    }

private:
    void waitRead() const
    {
        std::unique_lock<std::mutex> lock(m_blockingReadMutex);
        m_condition.wait(lock);
    }

    void resumeRead() const
    {
        std::unique_lock<std::mutex> lock(m_blockingReadMutex);
        m_condition.notify_one();
    }

    void waitWrite() const
    {
        std::unique_lock<std::mutex> lock(m_blockingWriteMutex);
        m_condition.wait(lock);
    }

    void resumeWrite() const
    {
        std::unique_lock<std::mutex> lock(m_blockingWriteMutex);
        m_condition.notify_one();
    }

protected:
    mutable std::mutex m_mutex;
    mutable std::mutex m_blockingReadMutex;
    mutable std::mutex m_blockingWriteMutex;
    mutable std::condition_variable m_condition;
    std::deque<T> m_deque;
};

