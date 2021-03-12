#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

using namespace std;

class ThreadPool {
   private:
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock{_mutex};
                while (running && tasks.empty()) {
                    cond.wait(lock);
                }
                /*
                cond.wait(lock, [this] {
                    return !running || !tasks.empty();
                });
                */
                if (!running && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

   public:
    explicit ThreadPool(size_t maxThreads) : running(true) {
        for (size_t i = 0; i < maxThreads; ++i) {
            threads.emplace_back([this]() { run(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            running = false;
        }
        cond.notify_all();
        for (auto &th : threads) {
            if (th.joinable()) th.join();
        }
    }

    template <class F, class... Args>
    auto add(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!running) throw std::runtime_error("threadpool is stopped!");
            tasks.emplace([task]() { (*task)(); });
        }
        cond.notify_one();
        return result;
    }

   private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;

    bool running;
    std::mutex _mutex;
    std::condition_variable cond;
};

#endif