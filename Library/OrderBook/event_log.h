#pragma once
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <thread>

#include "spsc_queue.h"
#include "Tracy.hpp"

template <typename T, size_t QueueCapacity, size_t BufferCapacity>
class event_log
{
public:
    event_log(const std::filesystem::path& path = "")
    {
        event_log::set_output_path(OUTPUT_DIR/path);
    }

public:
    void start()
    {
        _buffer_count = 0;
        _stop_flag = false;
        _worker_thread = std::thread(&event_log::_task, this);
    }
    void join()
    {
        _stop_flag = true;
        _queue.join();
        if (_worker_thread.joinable())
            _worker_thread.join();
    }

public:
    void set_output_path(const std::filesystem::path& path)
    {
        _output_path = path;
        std::filesystem::create_directories(_output_path.parent_path());
        std::ofstream(_output_path, std::ios::binary | std::ios::trunc);
    }
    void save(T& t)
    {
    #if TRACY_ENABLE == 1
        ZoneScopedN("save");
    #endif
        _queue.push(t);
    }
    void flush(const bool& yield = false)
    {
        {
            std::lock_guard lock(_mutex);
            _flush_done = false;
        }

        if (yield)
        {
            std::unique_lock lock(_mutex);
            _queue_empty_cv.wait(lock, [&] { return _queue.empty(); });
        }

        _flush_flag = true;
        _queue.skip();

        if (yield)
        {
            std::unique_lock lock(_mutex);
            _flush_done_cv.wait(lock, [&] { return _flush_done; });
        }
    }

private:
    void _task()
    {
        T t;
        while (true)
        {
            if (_queue.empty())
                _queue_empty_cv.notify_all();

            if (_queue.pop(t))
            {
                {
                #if TRACY_ENABLE == 1
                    ZoneScopedN("add to buffer");
                #endif
                    // std::cout << std::format("[{}] added to buffer (index: {})", _output_path.filename().string(), _buffer_count.load()) << std::endl;
                    _buffer[_buffer_count++] = std::move(t);
                }

                if (_buffer_count == BufferCapacity)
                    event_log::_write_to_disk();
            }
            if (_stop_flag == true)
            {
                event_log::_write_to_disk(true);
                break;
            }
            if (_flush_flag == true)
            {
                event_log::_write_to_disk(true);

                {
                    std::lock_guard lock(_mutex);
                    _flush_done = true;
                }
                _flush_done_cv.notify_all();
            }
        }
    }
    void _write_to_disk(const bool& flush = false)
    {
    #if TRACY_ENABLE == 1
        ZoneScopedN("write_to_disk");
    #endif
        std::ofstream out(_output_path, std::ios::binary | std::ios::app);

        out.write(reinterpret_cast<const char*>(_buffer.data()),
                  sizeof(T) * _buffer_count);

        if (flush)
            out.flush();

        // std::cout << std::format("[{}] wrote to disk (count: {})", _output_path.filename().string(), _buffer_count.load()) << std::endl;
        _buffer_count = 0;
    }

private:
    std::filesystem::path _output_path;
    spsc_queue<T, QueueCapacity> _queue;
    std::array<T, BufferCapacity> _buffer;

    std::atomic_size_t _buffer_count = 0;
    std::atomic_bool _stop_flag = false;
    std::atomic_bool _flush_flag = false;
    std::mutex _mutex;
    std::condition_variable _flush_done_cv;
    std::condition_variable _queue_empty_cv;
    std::thread _worker_thread;

    bool _flush_done = false;
};
