#pragma once
#include <array>
#include <mutex>
#include <semaphore>


template <typename T, size_t Capacity>
class spsc_queue
{
public:
    using size_type = std::size_t;
    using value_type = T;

public:
    bool push(const T& t)
    {
        const size_type head = _head;
        size_type next = (head + 1) % Capacity;
        if (next == _tail)
            return false;

        _data[head] = t;
        _head = next;
        _semaphore.release();
        return true;
    }
    bool pop(T& t)
    {
        _semaphore.acquire();

        const size_type tail = _tail;
        if (tail == _head || _stop_flag)
        {
            return false;
        }
        if (_skip_flag)
        {
            _skip_flag = false;
            return false;
        }

        t = _data[tail];
        _tail = (tail + 1) % Capacity;
        return true;
    }

public:
    void join()
    {
        _stop_flag = true;
        _semaphore.release();
    }
    void skip()
    {
        _skip_flag = true;
        _semaphore.release();
    }


public:
    bool empty() const { return _head == _tail; }

private:
    alignas(std::hardware_destructive_interference_size) std::atomic<size_type> _head = 0;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_type> _tail = 0;
    std::counting_semaphore<Capacity> _semaphore{0};
    std::array<T, Capacity> _data;
    std::atomic_bool _stop_flag = false;
    std::atomic_bool _skip_flag = false;
};
